#include "timer_system.h"
#include "lib_log.h"
#include "lib_time_source.h"
#include "linux_like_bitops.h"

IMPLEMENT_IDCREATE_WITHTYPE(TimerSystem, EOT_OBJ_TIMER_SYSTEM, CObj)

TimerSystem::TimerSystem() {
  if (SHM_MODE_INIT == get_shm_mode()) {
    CreateInit();
  } else {
    ResumeInit();
  }
}

void TimerSystem::CreateInit() {
  memset(&tv1_, -1, sizeof(tv1_));
  memset(&tv2_, -1, sizeof(tv2_));
  memset(&tv3_, -1, sizeof(tv3_));
  memset(&tv4_, -1, sizeof(tv4_));
  memset(&tv5_, -1, sizeof(tv5_));
}

TimerSystem::~TimerSystem() {
  int j;
  for (j = 0; j < TVN_SIZE; j++) {
    if (tv5_.vec[j] >= 0)
      CIDRuntimeClass::DestroyObj(Timer::GetObjectByID(tv5_.vec[j]));
    if (tv4_.vec[j] >= 0)
      CIDRuntimeClass::DestroyObj(Timer::GetObjectByID(tv4_.vec[j]));
    if (tv3_.vec[j] >= 0)
      CIDRuntimeClass::DestroyObj(Timer::GetObjectByID(tv3_.vec[j]));
    if (tv2_.vec[j] >= 0)
      CIDRuntimeClass::DestroyObj(Timer::GetObjectByID(tv2_.vec[j]));
  }
  for (j = 0; j < TVR_SIZE; j++) {
    if (tv1_.vec[j] >= 0)
      CIDRuntimeClass::DestroyObj(Timer::GetObjectByID(tv1_.vec[j]));
  }
  printf("TimerSystem destory\n");
}

int TimerSystem::Init(int64_t jiffies) {
  int j;
  for (j = 0; j < TVN_SIZE; j++) {
    tv5_.vec[j] = Timer::CreateInitListHead()->GetObjectID();
    tv4_.vec[j] = Timer::CreateInitListHead()->GetObjectID();
    tv3_.vec[j] = Timer::CreateInitListHead()->GetObjectID();
    tv2_.vec[j] = Timer::CreateInitListHead()->GetObjectID();
  }
  for (j = 0; j < TVR_SIZE; j++) {
    tv1_.vec[j] = Timer::CreateInitListHead()->GetObjectID();
  }

  timer_jiffies_ = jiffies;
  next_timer_ = timer_jiffies_;
  active_timers_ = 0;
  all_timers_ = 0;
  return 0;
}

void TimerSystem::DoInternalAddTimer(Timer *timer) {
  int64_t expires = timer->Expires();
  int64_t idx = expires - timer_jiffies_;
  int32_t vec;

  if (idx < TVR_SIZE) {
    int i = expires & TVR_MASK;
    vec = tv1_.vec[i];
  } else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
    int i = (expires >> TVR_BITS) & TVN_MASK;
    vec = tv2_.vec[i];
  } else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
    int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
    vec = tv3_.vec[i];
  } else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
    int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
    vec = tv4_.vec[i];
  } else if ((signed long)idx < 0) {
    // Can happen if you add a timer with expires == jiffies,
    // or you set a timer to go off in the past
    vec = tv1_.vec[(timer_jiffies_ & TVR_MASK)];
  } else {
    int i;
    // If the timeout is larger than MAX_TVAL (on 64-bit
    // architectures or with CONFIG_BASE_SMALL=1) then we
    // use the maximum timeout.
    if (idx > MAX_TVAL) {
      idx = MAX_TVAL;
      expires = idx + timer_jiffies_;
    }
    i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
    vec = tv5_.vec[i];
  }
  // Timers are FIFO:
  Timer *timer_list = Timer::GetObjectByID(vec);
  timer->ListAddTail(timer_list);
}

bool TimerSystem::CatchupTimerJiffies(int64_t jiffies) {
  if (!all_timers_) {
    timer_jiffies_ = jiffies;
    return true;
  }
  return false;
}

void TimerSystem::InternalAddTimer(Timer *timer, int64_t jiffies) {
  (void)CatchupTimerJiffies(jiffies);
  DoInternalAddTimer(timer);

  if (!active_timers_++ || timer->Expires() < next_timer_)
    next_timer_ = timer->Expires();
  all_timers_++;
}

int TimerSystem::DetachIfPending(Timer *timer, bool clear_pending, int64_t jiffies) {
  if (!timer->TimerPending())
    return 0;

  timer->DetachTimer(clear_pending);
  active_timers_--;
  if (timer->Expires() == next_timer_)
    next_timer_ = timer_jiffies_;
  all_timers_--;
  (void)CatchupTimerJiffies(jiffies);
  return 1;
}

int TimerSystem::InternalModTimer(Timer *timer, int64_t jiffies, int64_t expires,
                                  bool pending_only) {
  int ret = 0;

  ret = DetachIfPending(timer, false, jiffies);
  if (!ret && pending_only)
    return ret;

  timer->SetExpires(expires);
  InternalAddTimer(timer, jiffies);

  // printf("InternalModTimer:%s\n", timer->DebugString().c_str());

  return ret;
}

// mod_TimerPending - modify a pending timer's timeout
// @timer: the pending timer to be modified
// @expires: new timeout in jiffies
// mod_TimerPending() is the same for pending timers as ModTimer(),
// but will not re-activate and modify already deleted timers.
// It is useful for unserialized use of timers.
int TimerSystem::ModTimerPending(Timer *timer, int64_t jiffies, int64_t expires) {
  return InternalModTimer(timer, jiffies, expires, true);
}

// ModTimer - modify a timer's timeout
// @timer: the timer to be modified
// @expires: new timeout in jiffies
// ModTimer() is a more efficient way to update the expire field of an
// active timer (if the timer is inactive it will be activated)
// ModTimer(timer, expires) is equivalent to:
//     DelTimer(timer); timer->expires = expires; AddTimer(timer);
// Note that if there are multiple unserialized concurrent users of the
// same timer, then ModTimer() is the only safe way to modify the timeout,
// since AddTimer() cannot modify an already running timer.
// The function returns whether it has modified a pending timer or not.
// (ie. ModTimer() of an inactive timer returns 0, ModTimer() of an
// active timer returns 1.)
int TimerSystem::ModTimer(Timer *timer, int64_t jiffies, int64_t expires) {
  // This is a common optimization triggered by the
  // networking code - if the timer is re-modified
  // to be the same thing then just return:
  if (timer->TimerPending() && timer->Expires() == expires)
    return 1;

  return InternalModTimer(timer, jiffies, expires, false);
}

// AddTimer - start a timer
// @timer: the timer to be added
// The kernel will do a ->function(@timer) callback from the
// timer interrupt at the ->expires point in the future. The
// current time is 'jiffies'.
// The timer's ->expires, ->function fields must be set prior calling this
// function.
// Timers with an ->expires field in the past will be executed in the next
// timer tick.
void TimerSystem::AddTimer(Timer *timer, int64_t jiffies) {
  assert(!timer->TimerPending());
  ModTimer(timer, jiffies, timer->Expires());
}

// DelTimer - deactivate a timer.
// @timer: the timer to be deactivated
// DelTimer() deactivates a timer - this works on both active and inactive
// timers.
// The function returns whether it has deactivated a pending timer or not.
// (ie. DelTimer() of an inactive timer returns 0, DelTimer() of an
// active timer returns 1.)
int TimerSystem::DelTimer(Timer *timer, int64_t jiffies) {
  int ret = 0;

  if (timer->TimerPending()) {
    ret = DetachIfPending(timer, true, jiffies);
  }

  return ret;
}

void TimerSystem::DetachExpiredTimer(Timer *timer, int64_t jiffies) {
  // printf("DetachExpiredTimer:%s\n", timer->DebugString().c_str());
  timer->DetachTimer(true);
  active_timers_--;
  all_timers_--;
  CatchupTimerJiffies(jiffies);
}

int TimerSystem::Cascade(struct tvec *tv, int index) {
  // Cascade all the timers from tv up one level
  // Timer *tv_list = Timer::CreateInitListHead();
  Timer *tv_list = Timer::CreateInitListHead();

  Timer *old_list = Timer::GetObjectByID(tv->vec[index]);
  old_list->ListReplaceInit(tv_list);

  // We are removing _all_ timers from the list, so we
  // don't have to detach them individually.
  Timer *timer = tv_list->GetNextObject();
  while (timer != tv_list) {
    Timer *next = timer->GetNextObject();
    DoInternalAddTimer(timer);
    timer = next;
  }

  tv_list->Destroy();

  return index;
}

#define INDEX(N) ((timer_jiffies_ >> (TVR_BITS + (N)*TVN_BITS)) & TVN_MASK)

// __run_timers - run all expired timers (if any)
// This function Cascades all vectors and executes all expired timer
// vectors.
void TimerSystem::RunTimers(int64_t jiffies) {
  if (CatchupTimerJiffies(jiffies)) {
    return;
  }
  Timer *work_list = Timer::CreateInitListHead();
  while (jiffies >= timer_jiffies_) {
    int index = ((uint64_t)timer_jiffies_) & TVR_MASK;
    // Cascade timers:
    if (!index && (!Cascade(&tv2_, INDEX(0))) && (!Cascade(&tv3_, INDEX(1))) &&
        !Cascade(&tv4_, INDEX(2)))
      Cascade(&tv5_, INDEX(3));

    ++timer_jiffies_;
    Timer *timer_list = Timer::GetObjectByID(tv1_.vec[index]);
    timer_list->ListReplaceInit(work_list);
    while (!work_list->ListEmpty()) {
      Timer *timer = work_list->GetNextObject();
      ExpiryAction *action = timer->Action();
      int64_t data = timer->UserData();
      DetachExpiredTimer(timer, jiffies);
      if (action) {
        action->OnExpiry(timer->GetGlobalID(), data);
      }
      if (0 == timer->Interval()) {
        CIDRuntimeClass::DestroyObj(timer);
      } else {
        timer->SetExpires(timer->Expires() + timer->Interval());
        DoInternalAddTimer(timer);
        if (!active_timers_++ || timer->Expires() < next_timer_)
          next_timer_ = timer->Expires();
        all_timers_++;
      }
    }
  }

  work_list->Destroy();
}

int TimerSystem::SetTimer(ExpiryAction *action, int64_t expires, int64_t interval /* = 0*/,
                          int64_t user_data /* = 0*/) {
  Timer *timer = dynamic_cast<Timer *>(CIDRuntimeClass::CreateObj(EOT_OBJ_TIMER));
  if (!timer) {
    return INVALID_ID;
  }

  if (expires < 0) {
    expires = 0;
  }
  if (interval < 0) {
    interval = 0;
  }

  timer->Init(action, GetRealTickTimeMs() + expires, interval, user_data);
  AddTimer(timer, GetRealTickTimeMs());
  GetRealTickTimeMs();

  return timer->GetGlobalID();
}

int TimerSystem::ClearTimer(int timer_id) {
  Timer *timer =
      dynamic_cast<Timer *>(CIDRuntimeClass::GetObjFromGlobalID(timer_id, EOT_OBJ_TIMER));
  if (!timer) {
    return -1;
  }

  DelTimer(timer, GetRealTickTimeMs());
  CIDRuntimeClass::DestroyObj(timer);

  return 0;
}

int TimerSystem::ResetTimer(int timer_id, ExpiryAction *action, int64_t expires,
                            int64_t interval /* = 0*/, int64_t user_data /* = 0*/) {
  Timer *timer =
      dynamic_cast<Timer *>(CIDRuntimeClass::GetObjFromGlobalID(timer_id, EOT_OBJ_TIMER));
  if (!timer) {
    return -1;
  }

  if (expires < 0) {
    expires = 0;
  }
  if (interval < 0) {
    interval = 0;
  }

  DelTimer(timer, GetRealTickTimeMs());
  timer->Init(action, GetRealTickTimeMs() + expires, interval, user_data);
  AddTimer(timer, GetRealTickTimeMs());
  return 0;
}
