// write like https://github.com/torvalds/linux/blob/master/include/linux/list.h
// Double linked lists with a single pointer list head.
// Mostly useful for hash tables where the two pointer list head is
// too wasteful.
// You lose the ability to access the tail in O(1).
//
// can access by obj_id and remove from list in O(1).
//
// @author justinzhu
//@date 2022年7月3日16:19:35

#pragma once

#include "comm_object.h"
#include "lib_str.h"

static const int32_t LIST_POISON = INVALID_ID;
static const int32_t LIST_POISON_1 = LIST_POISON - 1;
static const int32_t LIST_POISON_2 = LIST_POISON - 2;

template <typename T>
class ListHead {
 public:
  ListHead() {
    if (SHM_MODE_INIT == get_shm_mode()) {
      CreateInit();
    } else {
      ResumeInit();
    }
  }
  virtual ~ListHead() {}
  void CreateInit() { next_ = prev_ = LIST_POISON; }
  void ResumeInit() {}

  std::string DebugString() const {
    return format_string("(self:%d, prev:%d, next:%d)", Self(), Prev(), Next());
  }

 public:
  static T *CreateInitListHead() {
    T *obj = dynamic_cast<T *>(T::CreateObject());
    obj->InitListHead();
    return obj;
  }

  void Destroy() { CIDRuntimeClass::DestroyObj(GetObject()); }
  void InitListHead() { next_ = prev_ = Self(); }

 public:
  int32_t Next() const { return next_; }
  int32_t Prev() const { return prev_; }
  int32_t Self() { return dynamic_cast<T *>(this)->GetObjectID(); }
  void SetNext(int32_t next) { next_ = next; }
  void SetPrev(int32_t prev) { prev_ = prev; }

  T *GetNextObject() const { return T::GetObjectByID(Next()); }
  T *GetPrevObject() const { return T::GetObjectByID(Prev()); }
  T *GetObject() { return dynamic_cast<T *>(this); }

 public:
  // list_empty - tests whether a list is empty
  // @head: the list to test.
  bool ListEmpty() { return Next() == Self(); }

  // list_is_singular - tests whether a list has just one entry.
  // @head: the list to test.
  bool ListIsSingular() const { return !ListEmpty() && (Next() == Prev()); }

 public:
  // list_add - add a new entry
  // @self: new entry to be added
  // @head: list head to add it after
  // Insert a new entry after the specified head.
  // This is good for implementing stacks.
  void ListAdd(ListHead *head) { InternalListAdd(head, head->GetNextObject()); }

  // list_add_tail - add a new entry
  // @self: new entry to be added
  // @head: list head to add it before
  // Insert a new entry before the specified head.
  // This is useful for implementing queues.
  void ListAddTail(ListHead *head) { InternalListAdd(head->GetPrevObject(), head); }

  // list_del - deletes entry from list.
  // @self: the element to delete from the list.
  // Note: list_empty() on entry does not return true after this, the entry is
  // in an undefined state.
  void ListDelEntry() { InternalListDel(GetPrevObject(), GetNextObject()); }

  void ListDel() {
    InternalListDel(GetPrevObject(), GetNextObject());
    SetNext(LIST_POISON_1);
    SetPrev(LIST_POISON_1);
  }

  // list_del_init - deletes entry from list and reinitialize it.
  // @self: the element to delete from the list.
  void LisDelInit() {
    ListDelEntry();
    InitListHead();
  }

  // list_replace - replace old entry by new one
  // @self : the element to be replaced
  // @new : the new element to insert
  // If @self was empty, it will be overwritten.
  void ListReplace(ListHead *new_head) {
    if (ListEmpty()) {
      return;
    }
    new_head->SetNext(Next());
    new_head->GetNextObject()->SetPrev(new_head->Self());
    new_head->SetPrev(Prev());
    new_head->GetPrevObject()->SetNext(new_head->Self());
  }

  void ListReplaceInit(ListHead *new_head) {
    ListReplace(new_head);
    InitListHead();
  }

 protected:
  // Insert a new entry between two known consecutive entries.
  // This is only for internal list manipulation where we know
  // the prev/next entries already!
  void InternalListAdd(ListHead *prev, struct ListHead *next) {
    next->SetPrev(Self());
    SetNext(next->Self());
    SetPrev(prev->Self());
    prev->SetNext(Self());
  }

  // Delete a list entry by making the prev/next entries
  // point to each other.
  // This is only for internal list manipulation where we know
  // the prev/next entries already!
  static void InternalListDel(ListHead *prev, ListHead *next) {
    next->SetPrev(prev->Self());
    prev->SetNext(next->Self());
  }

 private:
  // all store obj_id;
  int32_t next_;
  int32_t prev_;
};
