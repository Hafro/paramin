#ifndef datastructure_h
#define datastructure_h

#include "paramin.h"

class myLink {
public:
  myLink* l;
  int tid;
};

/**
 * \class Queue 
 * \brief class queue stores data items of type integer. Items can be added to the end or front of the queue and removed from the end or the front 
*/

class queue {
private:
  int numberInQueue;
  myLink* first;
  myLink* last;
public:
  queue();
  ~queue();
  void put(int tid);
  int get();
  int isEmpty();
  int getNumItems();
  int getLast();
  void putFirst(int dataId);
  int contains(int id);
};

#endif
