#include "datastructure.h"

queue::queue() {
  first = NULL;
  last = NULL;
  numberInQueue = 0;
}

queue::~queue() {
  int temp;
  while (!isEmpty())
    temp = get();
}

int queue::isEmpty() {
  return (first == NULL);
}

int queue::getNumberOfItems() {
  return numberInQueue;
}

void queue::put(int tid) {
  if (first) {
    myLink* temp = new myLink;
    last->l = temp;
    last = temp;
    last->tid = tid;
    last->l = NULL;
  } else {
    first = new myLink;
    last = first;
    first->tid = tid;
    last->l = NULL;
  }
  numberInQueue++;
}

int queue::get() {
  assert(first);
  int tid = first->tid;
  if (first == last) {
    delete last;
    last = NULL;
    first = NULL;
  } else {
    myLink* temp = first->l;
    delete first;
    first = temp;
  }
  numberInQueue--;
  return tid;
}

int queue::getLast() {
  assert(last);
  int tid = last->tid;
  if (first == last) {
    delete last;
    last = NULL;
    first = NULL;
  } else {
    myLink* temp = last->l;
    delete last;
    last = temp;
  }
  numberInQueue--;
  return tid;
}

void queue::putFirst(int dataId) {
  if (first) {
    // the queue is not empty
    myLink* temp = new myLink;
    temp->l = first;
    first = temp;
    first->tid = dataId;
  } else {
    // making first link in queue
    first = new myLink;
    last = first;
    first->tid = dataId;
    last->l = NULL;
  }
  numberInQueue++;
}

int queue::contains(int id) {
  int i = 0;
  int inQueue = 0;
  myLink* temp;
  if (isEmpty())
    return 0;

  temp = first;
  while (inQueue == 0 && i < numberInQueue) {
    assert(temp != NULL);
    inQueue = (temp->tid == id);
    temp = temp->l;
    i++;
  }
  return inQueue;
}
