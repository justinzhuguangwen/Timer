
#include "timer.h"

IMPLEMENT_IDCREATE_WITHTYPE(Timer, EOT_OBJ_TIMER, CObj)

Timer::Timer() {
  if (SHM_MODE_INIT == get_shm_mode()) {
    CreateInit();
  } else {
    ResumeInit();
  }
}

void Timer::CreateInit() {
  action_ = nullptr;
  expires_ = 0;
  interval_ = 0;
  user_data_ = 0;
}

void Timer::ResumeInit() {
  char *tmp = reinterpret_cast<char *>(action_);
  tmp += CSharedMem::GetSharedMem()->GetAddrOffset();
  action_ = reinterpret_cast<ExpiryAction *>(tmp);
}