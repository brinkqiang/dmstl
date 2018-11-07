/*******************************************************
 *                                                     *
 *                                                     *
 * containerFactory.h :                                *
 * Grum Ketema                                         *
 * August 5, 2002                                      *
 * This memory allocator is Based on the STL allocator *
 * found in The C++ Programming language Third Edition *
 * Section 19.4 (Page 567)                             *
 *                                                     *
 *******************************************************/

#ifndef CONTAINERFACTORY_H_D
#define CONTAINERFACTORY_H_D
#include "gallocator.h"
#include "gdefine.h"
#include <memory>
#include <map>

struct keyComp {
	bool operator()(const char* key1,const char* key2)
	{
		return(strcmp(key1,key2)<0);
	}
};
class containerMap: public map<char*,void*,keyComp,SharedAllocator<char* > > {};
class containerFactory {
	public:
		containerFactory():pool_(sizeof(containerMap)){}
		~containerFactory() {}
		template<class Container> Container* createContainer(char* key,Container* c=NULL);
		template<class Container> Container* getContainer(char* key,Container* c=NULL);
		template<class Container> int removeContainer(char* key,Container* c=NULL); 
	private:
		Pool pool_;
		int lock_()
		{
			return(pool_.lockContainer());
		}
		int unlock_()
		{
			return(pool_.unLockContainer());
		}
};

template<class Container> Container*
containerFactory::getContainer(char* key,Container* c)
{
	if (!key) {
		return(NULL);
	}
	if (this->lock_()<0) {
		perror("getContainer");
		return(NULL);
	}

	containerMap* mp = pool_.getContainer();
	if (!mp) {
		this->unlock_();
		return(NULL);
	}

	containerMap::iterator it = mp->find(key);
	if (it==mp->end()) {
		this->unlock_();
		return(NULL);
	}
	this->unlock_();
	return((Container*)it->second);
	//return (reinterpret_cast<Container*>(&(*it)));
}

template<class Container> Container*
containerFactory::createContainer(char*key,Container* c)
{
	if (!key) {
		return(NULL);
	}
	if (this->lock_()<0) {
		perror("createContainer");
		return(NULL);
	}

	containerMap* mp = pool_.getContainer();
	void* p=NULL;
	Container* cp = NULL;
	if (!mp) {
			//Allocate memory using shared memory allocator
			p = pool_.alloc(1);
			if (!p) {
				this->unlock_();
				return(NULL);
			}
			//construct the containerMap using placement new on the
			//allocated memory
			
			mp = new(p) containerMap;
			pool_.setContainer(mp);
	} else {
		cp = this->getContainer(key,c);
		if (cp) {
			this->unlock_();
			return(cp);
		}
	}
	Pool contPool(sizeof(Container));
	p = contPool.alloc(1);
	if (!p) {
		this->unlock_();
		return(NULL);
	}

	cp = new(p) Container;
	(*mp)[key] = reinterpret_cast<void*>(cp);
	this->unlock_();
	return(cp);
}
template<class Container> int
containerFactory::removeContainer(char*key, Container* c)
{

	if (!key) {
		return(0);
	}
	if (this->lock_()<0) {
		perror("removeContainer");
		return(0);
	}
	containerMap* mp = pool_.getContainer();
	if (!mp) {
		this->unlock_();
		return(0);
	}
	Container* cp = this->getContainer(key,c);
	if (!cp) {
		return(0);
	}
	size_t n = mp->erase(key);

	SharedAllocator<Container> alloc = cp->get_allocator();
	alloc.destroy(cp);
	alloc.deallocate(cp,1);
	/*cp->~Container();
	Pool contPool(sizeof(Container));
	contPool.free(cp,sizeof(Container));*/
	this->unlock_();
	return(n);
}
#endif //CONTAINERFACTORY_H
