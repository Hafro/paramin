#include "datastructure.h"

Queue::Queue() {
  first = NULL;
  last = NULL;
  numberInQueue = 0;
}

Queue::~Queue() {
  int temp;
  while (!isEmpty())
    temp = get();
}

int Queue::isEmpty() {
  return (first == NULL);
}

int Queue::getNumItems() {
  return numberInQueue;
}

void Queue::put(int tid) {
  if (first) {
    Link* temp = new Link;
    last->l = temp;
    last = temp;
    last->tid = tid;
    last->l = NULL;
  } else {
    first = new Link;
    last = first;
    first->tid = tid;
    last->l = NULL;
  }

  numberInQueue++;
}

int Queue::get() {
  assert(first);
  int tid = first->tid;
  if (first == last) {
    delete last;
    last = NULL;
    first = NULL;
  } else {
    Link* temp = first->l;
    delete first;
    first = temp;
  }
  numberInQueue--;
  return tid;
}

int Queue::getLast() {
  assert(last);
  int tid = last->tid;
  if (first == last) {
    delete last;
    last = NULL;
    first = NULL;
  } else {
    Link* temp = last->l;
    delete last;
    last = temp;
  }
  numberInQueue--;
  return tid;
}

void Queue::putFirst(int dataID) {
  if (first) {
    // the queue is not empty
    Link* temp = new Link;
    temp->l = first;
    first = temp;
    first->tid = dataID;
  } else {
    // making first link in queue
    first = new Link;
    last = first;
    first->tid = dataID;
    last->l = NULL;
  }
  numberInQueue++;
}

int Queue::contains(int id) {
  int i = 0;
  int inQueue = 0;
  Link* temp;
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
