/*******************************************************
 *                                                     *
 *                                                     *
 * allocator.h :                                       *
 * Grum Ketema                                         *
 * August 5, 2002                                      *
 * This memory allocator is Based on the STL allocator *
 * found in The C++ Programming language Third Edition *
 * Section 19.4 (Page 567)                             *
 *                                                     *
 *******************************************************/

#ifndef SHAREDALLOC_D_
#define SHAREDALLOC_D_
#include <sys/sem.h>
#include <string>
#include <stddef.h>
#include <fstream>
#include <iostream>
using namespace std;
class containerMap;
int lock(int);
int unlock(int);
class Pool {
	private:
		class shmPool {
			private:
				struct Container {
					containerMap* cont;
					Container()
					{
						cont=NULL;
					}
				};
				class Chunk {
					private:
						struct Link {
							size_t size;
							Link* next;
							Link()
							{
								size=0;
								next=NULL;
							}
						};
					public:
						Chunk():head_(NULL),
							semId_(-1),
							shmId_(-1) {}
						Chunk(Chunk&);
						~Chunk() {}
						struct shmid_ds* p_buf;
						void* alloc(size_t size);
						void print(ostream& os);
						void free (void* p,size_t size);
						void init(int sz,int setContFlag);
						void semid(int semid) {semId_ = semid;}
						int semid() { return(semId_);}
						void shmid(int shmid) {shmId_ = shmid; }
						int shmid() {return(shmId_);}
						static size_t ALIGNED(size_t);
					private:
						int shmId_;
						int semId_;
						Link* head_;
						int lock_()
						{
							return(::lock(semId_));
						}
						int unlock_()
						{
							return(::unlock(semId_));
						}
						void merge_(Link* v);
				};
				void init_();
				int key_;
				char* path_;
				Chunk** chunks_;
				size_t segs_;
				size_t segSize_;
				Container* contPtr_;
				int contSemId_;
			public:
				shmPool();
				~shmPool();
				size_t maxSize();
				void* alloc(size_t size);
				void free(void* p, size_t size);
				void print(ostream& os);
				int shmPool::lockContainer()
				{
					return(::lock(contSemId_));
				}
	
				int unLockContainer()
				{
					return(::unlock(contSemId_));
				}
				containerMap* getContainer()
				{
					return(contPtr_?contPtr_->cont:NULL);
				}
				void shmPool::setContainer(containerMap* container)
				{
					if (contPtr_) {
						contPtr_->cont=container;
					}
				}
		};

	private:
		static shmPool shm_;
		size_t elemSize_;
	public:
		Pool(size_t elemSize):
			elemSize_(shmPool::Chunk::ALIGNED(elemSize)) {}
		~Pool() {}
		size_t maxSize()
		{
			return(shm_.maxSize());
		}
		void* alloc(size_t size)
		{
			return(shm_.alloc(size*elemSize_));
		}
		void free(void* p, size_t size)
		{
			shm_.free(p,size*elemSize_);
		}
		bool compare(const Pool& p) const
		{
			return(p.elemSize_==this->elemSize_);
		}

		int lockContainer()
		{
			return(shm_.lockContainer());
		}

		int unLockContainer()
		{
			return(shm_.unLockContainer());
		}

		containerMap* getContainer()
		{
			return(shm_.getContainer());
		}
		void setContainer(containerMap* container)
		{
			shm_.setContainer(container);
		}

		void print(ostream& os)
		{
			os<<"Element Size["<<elemSize_<<"]"<<endl;
			shm_.print(os);
		}

		static struct sembuf opLock_[2];
		static struct sembuf opUnLock_[1];
};
inline bool operator==(const Pool& a,const Pool& b)
{
	return(a.compare(b));
}


template<class T>class SharedAllocator {
	private:
		Pool pool_;	// pool of elements of sizeof(T)
	public:
		typedef T value_type;
		typedef unsigned int  size_type;
		typedef ptrdiff_t difference_type;
   
		typedef T* pointer;
		typedef const T* const_pointer;
		
		typedef T& reference;
		typedef const T& const_reference;
    
		pointer address(reference r) const { return &r; }
		const_pointer address(const_reference r) const {return &r;}
    
		SharedAllocator() throw():pool_(sizeof(T)) {}
		template<class U> SharedAllocator(const SharedAllocator<U>& t) throw():
						pool_(sizeof(T)) {}
		~SharedAllocator() throw() {};
   
		pointer allocate(size_t n, const void* hint=0)	// space for n Ts
		{
			return(static_cast<pointer> (pool_.alloc(n)));
		}
		void deallocate(pointer p,size_type n) // deallocate n Ts, don't destroy
		{
			pool_.free((void*)p,n);
			return;
		}
    
		void construct(pointer p, const T& val) { new(p) T(val); } // initialize *p by val
		void destroy(pointer p) { p->~T(); } // destroy *p but don't deallocate
    
		size_type max_size() const throw()
		{
			pool_.maxSize();
		}
    
		template<class U>	
		struct rebind { typedef SharedAllocator<U> other; }; // in effect: typedef SharedAllocator<U> other
};

template<class T>bool operator==(const SharedAllocator<T>& a, const SharedAllocator<T>& b) throw()
{
		return(a.pool_ == b.pool_);
}
template<class T>bool operator!=(const SharedAllocator<T>& a, const SharedAllocator<T>& b) throw()
{
		return(!(a.pool_ == b.pool_));
}
  
#endif //SHAREDALLOC_
