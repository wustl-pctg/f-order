#ifndef _ferret_piper_h_
#define _ferret_piper_h_

#include <sys/types.h>
#include <dirent.h>
#include <stack>

#include "cilk/future.h"

class filter_load {
	char m_path[BUFSIZ];
	const char *m_single_file;
	
	std::stack<DIR *> m_dir_stack;
	std::stack<int>   m_path_stack;
	
	private:
		void push_dir(const char * dir);
	
	public:
		filter_load(const char * dir);
		/*override*/void* operator()( void* item );
};


class filter_seg {
	public:
		  filter_seg();
		  /*override*/void* operator()(void* item);
};

class filter_extract {
	public:
		filter_extract();
		/*override*/void* operator()(void* item);
};

class filter_vec {
	public:
		filter_vec();
		/*override*/void* operator()(void* item);
};

class filter_rank {
	public:
		filter_rank();
		/*override*/void* operator()(void* item);
};

class filter_out {
	public:
		filter_out();
		/*override*/void operator()(cilk::future<void>* prev, void* item);
};

#endif
