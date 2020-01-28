/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors: Yifan Xu <xuyifan@wustl.edu>
 *          I-Ting Angelina Lee <angelee@wustl.edu>
 *          Jim Sukha <jim.sukha@intel.com>
**/

/**
 * A hacked-together header file for converting a TBB pipeline into a
 * Piper implementation.
 *
 * The idea is to replace tbb::filter with cilk::piper::filter
 * and tbb::pipeline with cilk::piper::pipeline.
 * and have things "just work".
 */


#ifndef __PIPER_TBB_COMPAT_H_
#define __PIPER_TBB_COMPAT_H_

#include <vector>
#include <cilk/piper.h>

namespace cilk {
    namespace piper {

	class filter {
	    
	protected:
	    // True if this filter is serial.
	    // False if it is a parallel filter.
	    const bool m_is_serial_filter;

	    // Construct the filter. 
	    filter( bool is_serial) : m_is_serial_filter(is_serial) { }

	public:
	    bool is_serial() {
		return m_is_serial_filter;
	    }
	    
	    // Operator for the filter.  Should take in an input item,
	    // and return an output item.
	    virtual void* operator() (void* item) = 0;

	    static const bool SERIAL_IN_ORDER = 1;
	    static const bool SERIAL = 1;
	    static const bool PARALLEL = 0;
	};


	class pipeline {
	private:
	    friend class filter;


	    
	    
	public:
	    pipeline() {
	    }
	    
	    void clear() {
		m_filter_list.clear();
	    }
	    
	    ~pipeline() {
		clear();
	    }
	    
	    std::vector<filter*> m_filter_list;
	    
	    void add_filter(filter& f) {
		m_filter_list.push_back(&f);
	    }

	    void run(size_t throttle_limit) {
		bool done = false;

		if (m_filter_list.size() > 0) {
		    CILK_PIPE_WHILE_BEGIN_THROTTLED(!done, throttle_limit) {
			void* item = (*(m_filter_list[0]))(NULL);
			int64_t stage_num = 1;

			if (item) {
			    while (stage_num < m_filter_list.size()) {
				// Begin the next stage with the
				// appropriate stage boundary.
				if ( (*(m_filter_list[stage_num])).is_serial()) {
				    // Serial stage begins with a wait
				    CILK_STAGE_WAIT(stage_num);
				}
				else {
				    // Parallel stage does not wait.
				    CILK_STAGE(stage_num);
				}
				// Execute the stage.
				item = (*(m_filter_list[stage_num]))(item);
			    
				// Increment the counter.
				stage_num++;
			    }

			    // The last filter should not generate any
			    // item to process.
			    assert(NULL == item);
			}
			else {
			    done = true;
			}
		    } CILK_PIPE_WHILE_END();
		}
	    }
	};	
    };
};


#endif // !defined(__PIPER_TBB_COMPAT_H_)
