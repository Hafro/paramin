#ifndef datastructure_h
#define datastructure_h

#include "paramin.h"

class Link {
public:
  Link* l;
  int tid;
};

/**
 * \class Queue 
 * \brief class Queue stores data items of type integer. Items can be added to the end or front of the queue and removed from the end or the front 
*/

class Queue {
private:
  int numberInQueue;
  Link* first;
  Link* last;
public:
  Queue();
  ~Queue();
  void put(int tid);
  int get();
  int isEmpty();
  int getNumItems();
  int getLast();
  void putFirst(int dataID);
  int contains(int id);
};

#endif
