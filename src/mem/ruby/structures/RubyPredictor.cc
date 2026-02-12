/*
 * Copyright (c) 2020 BSC
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/ruby/structures/RubyPredictor.hh"

#include "debug/RubyPredictor.hh"
#include "mem/ruby/system/RubySystem.hh"

namespace gem5
{

namespace ruby
{


RubyPredictor::RubyPredictor(const Params &p)
    :   SimObject(p),
        m_num_entries(p.num_entries),
        m_assoc(p.assoc),
        m_max_counter(p.max_counter),
        m_init_counter(p.init_counter),
        rubyPredictorStats(this)
{
    assert(m_num_entries > 0);

    m_num_sets = (m_num_entries / m_assoc);
    assert(m_num_sets > 1);

    m_num_set_bits = floorLog2(m_num_sets);
    assert(m_num_set_bits > 0);

    m_start_index_bit = 0; //Set by setController

    m_array.resize(m_num_sets,
        std::vector<PredictorEntry>(m_assoc));
}

RubyPredictor::
RubyPredictorStats::RubyPredictorStats(statistics::Group *parent)
    : statistics::Group(parent, "RubyPredictor"),
      ADD_STAT(numQueries, "Number of queriess observed"),
      ADD_STAT(numAllocatedEntries, "Number of addresses allocated for "
                                    "prediction"),
      ADD_STAT(numInvalObserved, "Number of invalidations to addr seen"),
      ADD_STAT(numEvictions, "Number of of conflicted entries")
{
}

int
RubyPredictor::queryPolicy(Addr address)
{
    DPRINTF(RubyPredictor, "Policy query for %#x\n", address);
    Addr line_addr = makeLineAddress(address);
    rubyPredictorStats.numQueries++;

    // check to see if we are tracking for this block
    PredictorEntry *entry = getPredictorEntry(line_addr);
    if (entry != NULL) {
        entry->m_use_time = m_controller->curCycle();
        return (int) (entry->m_count == 0);
    }

    // allocate a new entry
    initializeEntry(line_addr, getLRU(address));
    DPRINTF(RubyPredictor, "Allocated entry for predictor\n");
    // Cold Prediction TODO: Make it parametric
    return ((double) m_numNotReused / (double) m_numFetched) > 0.15;
}

void
RubyPredictor::observeFetch(Addr address)
{
    DPRINTF(RubyPredictor, "Observe Fetch AMO\n");
    Addr line_addr = makeLineAddress(address);
    m_numFetched++;

    // check to see if we are tracking AMO for this block
    PredictorEntry * entry = getPredictorEntry(line_addr);
    if (entry != NULL) {
        DPRINTF(RubyPredictor, "Observed Fetch for AMO %#x\n", address);
        entry->m_is_present = true;
        entry->m_is_reused = false;
    }
    // allocate a new AMO entry
    initializeEntry(line_addr, getLRU(address));
    DPRINTF(RubyPredictor, "Allocated entry for AMO\n");
}


void
RubyPredictor::observeReuse(Addr address)
{
    DPRINTF(RubyPredictor, "Observe Reuse AMO\n");
    Addr line_addr = makeLineAddress(address);

    // check to see if we are tracking AMO for this block
    PredictorEntry * entry = getPredictorEntry(line_addr);
    if (entry != NULL && entry->m_is_present) {
        DPRINTF(RubyPredictor, "Observed Reuse for AMO %#x\n", address);
        entry->m_is_reused = true;
    }
}


void
RubyPredictor::observeInvalidationEviction(Addr address)
{
    DPRINTF(RubyPredictor, "Observe Invalidation/Eviction\n");
    Addr line_addr = makeLineAddress(address);
    rubyPredictorStats.numInvalObserved++;

    // check to see if we are tracking AMO for this block
    PredictorEntry * entry = getPredictorEntry(line_addr);
    if (entry != NULL && entry->m_is_present) {
        DPRINTF(RubyPredictor, "Observed eviction for %#x\n", address);
        if (!entry->m_is_reused){
            DPRINTF(RubyPredictor, "Is not reused\n");
            if (entry->m_count > 0){
                entry->m_count--;
            }
            m_numNotReused++;
        } else {
            if (entry->m_count < m_max_counter){
                entry->m_count++;
            }
        }
        entry->m_is_present = false;
        DPRINTF(RubyPredictor, "Counter %d \n", entry->m_count);
    }
}


void
RubyPredictor::initializeEntry(Addr address, PredictorEntry * entry)
{
    DPRINTF(RubyPredictor, "Initialize entry\n");
    rubyPredictorStats.numAllocatedEntries++;

    // initialize the entry
    entry->m_address = makeLineAddress(address);
    entry->m_use_time = m_controller->curCycle();
    entry->m_count = m_init_counter;
    entry->m_is_valid = true;
    entry->m_is_reused = false;
    entry->m_is_present = false; //Set by ObserveFetch
}

int64_t
RubyPredictor::addressToSet(Addr address)
{
    assert(address == makeLineAddress(address));
    return bitSelect(address, m_start_index_bit,
                     m_start_index_bit + m_num_set_bits - 1);
}

PredictorEntry *
RubyPredictor::getLRU(Addr address)
{
    DPRINTF(RubyPredictor, "Get LRU\n");
    int64_t set = addressToSet(address);
    Cycles lru_access = m_array[set][0].m_use_time;
    if (!m_array[set][0].m_is_valid) {
        DPRINTF(RubyPredictor, "Set %d Way %d\n", set, 0);
        return &(m_array[set][0]);
    }
    PredictorEntry * lru_entry = &(m_array[set][0]);
    for (int i = 1; i < m_assoc; i++) {
        PredictorEntry * entry = &(m_array[set][i]);
        if (!entry->m_is_valid) {
            DPRINTF(RubyPredictor, "Set %d Way %d\n", set, i);
            return entry;
        }
        if (entry->m_use_time < lru_access) {
            lru_access = entry->m_use_time;
            lru_entry = entry;
        }
    }

    rubyPredictorStats.numEvictions++;

    return lru_entry;
}

PredictorEntry *
RubyPredictor::getPredictorEntry(Addr address)
{
    DPRINTF(RubyPredictor, "Get Predictor Entry\n");
    int64_t set = addressToSet(address);
    for (int i = 0; i < m_assoc; i++) {
        PredictorEntry* entry = &(m_array[set][i]);
        if (entry->m_is_valid && entry->m_address == address) {
            DPRINTF(RubyPredictor, "Set %d Way %d\n", set, i);
            return entry;
        }
    }
    return NULL;
}

void
RubyPredictor::print(std::ostream& out) const
{
    out << name() << " Predictor State\n";

    // print out allocated stream buffers
    out << "Entries:\n";
    for (int i = 0; i < m_num_sets; i++){
        for (int j = 0; j < m_assoc; j++){
            out << m_array[i][j].m_is_valid << " "
                << m_array[i][j].m_address  << " "
                << m_array[i][j].m_count    << " "
                << m_array[i][j].m_use_time << std::endl;
        }
    }
}

Addr
RubyPredictor::makeLineAddress(Addr addr) const
{
    return ruby::makeLineAddress(addr,
                           m_controller->m_ruby_system->getBlockSizeBits());
}

} // namespace ruby
} // namespace gem5
