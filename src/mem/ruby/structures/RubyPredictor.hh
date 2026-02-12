/*
 * Copyright (c) 2025 BSC
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

#ifndef __MEM_RUBY_STRUCTURES_PREDICTOR_HH__
#define __MEM_RUBY_STRUCTURES_PREDICTOR_HH__


#include "base/statistics.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/slicc_interface/AbstractController.hh"
#include "mem/ruby/slicc_interface/RubyRequest.hh"
#include "mem/ruby/system/RubySystem.hh"
#include "params/RubyPredictor.hh"
#include "sim/sim_object.hh"
#include "sim/system.hh"

namespace gem5
{

namespace ruby
{

class PredictorEntry
{
    public:

        /// constructor
        PredictorEntry()
        {
            m_use_time = Cycles(0);
            m_is_valid = false;

        }

        //! The base address for the predictor
        Addr m_address;

        //! the last time that any predictor was used
        Cycles m_use_time;

        //! valid bit for each entry
        bool m_is_valid;

        //! Reuse Bit
        bool m_is_reused;

        //! Present Bit
        bool m_is_present;

        //! number of times done near AMO
        unsigned int long long m_count;
};

class RubyPredictor : public SimObject
{
    public:
        typedef RubyPredictorParams Params;

        RubyPredictor(const Params &p);
        ~RubyPredictor() = default;

        /**
         * Query a memory access from the cache.
         *
         * @param address   The physical address that accessed the cache.
         */
        int queryPolicy(Addr address);

        /**
         * Notify a fetch in the predictor
         *
         * @param address   The physical address that accessed the cache.
         */
        void observeFetch(Addr address);

        /**
         * Notify a reuse in the predictor
         *
         * @param address   The physical address that accessed the cache.
         */
        void observeReuse(Addr address);

        /**
         * Notify an invalidation or eviction in the predictor
         *
         * @param address   The physical address that accessed the cache.
         */
        void observeInvalidationEviction(Addr address);

        /**
         * Print out some statistics
         */
        void print(std::ostream& out) const;

        /** Set cache controller connecting to this predictor */
        void setController(AbstractController* _ctrl)
        {
            m_controller = _ctrl;
            m_start_index_bit =
                m_controller->m_ruby_system->getBlockSizeBits();
        }

    private:

        Addr makeLineAddress(Addr addr) const;

        //! get set of an addr
        int64_t addressToSet(Addr address);

        /**
         * Returns an unused predictor entry (or if all are used, returns the
         * least recently used (accessed) entry).
         * @return  The least recently predictor entry.
         */
        PredictorEntry * getLRU(Addr address);

        //! allocate a new entry at a specific index
        void initializeEntry(Addr address, PredictorEntry * entry);

        //! get pointer to the matching predictor entry,
        //! returns NULL if not found
        PredictorEntry* getPredictorEntry(Addr address);

        AbstractController *m_controller;

        //! number entries available
        uint32_t m_num_entries;

        //! associativity of the predictor
        uint32_t m_assoc;
        //! number of sets in the predictor
        uint32_t m_num_sets;
        uint32_t m_num_set_bits;
        uint32_t m_start_index_bit;

        uint32_t m_max_counter;
        uint32_t m_init_counter;

        //! an array of the active AMO Predictor Entries
        std::vector<std::vector<PredictorEntry>> m_array;

        // Cold Prediction Stats
        uint64_t m_numFetched;
        uint64_t m_numNotReused;

        struct RubyPredictorStats : public statistics::Group
        {
            RubyPredictorStats(statistics::Group *parent);

            //! Count of accesses to the predictor
            statistics::Scalar numQueries;
            //! Count of allocated entries
            statistics::Scalar numAllocatedEntries;
            //! Count of seen invalidations
            statistics::Scalar numInvalObserved;
            //! Count of conflicted entries
            statistics::Scalar numEvictions;
        } rubyPredictorStats;

};

} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_STRUCTURES_PREDICTOR_HH__
