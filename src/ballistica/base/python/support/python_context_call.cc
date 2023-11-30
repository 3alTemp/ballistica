// Released under the MIT License. See LICENSE for details.

#include "ballistica/base/python/support/python_context_call.h"

#include "ballistica/base/logic/logic.h"
#include "ballistica/core/python/core_python.h"
#include "ballistica/shared/foundation/event_loop.h"
#include "ballistica/shared/generic/utils.h"
#include "ballistica/shared/python/python.h"
#include "ballistica/shared/python/python_sys.h"

namespace ballistica::base {

PythonContextCall* PythonContextCall::current_call_{};

PythonContextCall::PythonContextCall(PyObject* obj_in) {
  assert(g_base->InLogicThread());
  // As a sanity test, store the current context ptr just to make sure it
  // hasn't changed when we run. Contexts dying should result in us getting
  // invalidated so this should always match if we are running.
  // if (g_buildconfig.debug_build()) {
  //   context_target_sanity_test_ = context_state_.Get();
  // }
  BA_PRECONDITION(PyCallable_Check(obj_in));
  object_.Acquire(obj_in);
  GetTrace();

  // Inform the context that we are being added to it. It may want to
  // grab a weak-ref to us and inform us when it is going down.
  if (auto* context = context_state_.Get()) {
    context->RegisterContextCall(this);
  }
}

PythonContextCall::~PythonContextCall() {
  if (!context_state_.IsExpired()) {
    // If our context still exists, let's use it while we take our stuff down
    // (we may be holding refs to actors or whatnot).
    base::ScopedSetContext ssc(context_state_);
    object_.Release();
  } else {
    // Otherwise go with an empty context I guess.
    base::ScopedSetContext ssc(base::ContextRef(nullptr));
    object_.Release();
  }
}

auto PythonContextCall::GetObjectDescription() const -> std::string {
  return "<PythonContextCall from " + file_loc_ + " at "
         + Utils::PtrToString(this) + ">";
}

void PythonContextCall::GetTrace() {
  // Grab the file/line now in case we error
  // (useful for debugging simple timers and callbacks and such).
  file_loc_ = Python::GetPythonFileLocation();
}

// Called by our owning context when it goes down.
// We should clear ourself out to be a no-op if we still happen to be called.
void PythonContextCall::MarkDead() {
  dead_ = true;
  object_.Release();
}

void PythonContextCall::Run(PyObject* args) {
  assert(this);

  // We implicitly use core globals; don't normally do this.
  assert(g_core);

  if (dead_ || context_state_.IsExpired()) {
    return;
  }

  // Restore the context from when we were made.
  base::ScopedSetContext ssc(context_state_);

  // Hold a ref to this call throughout this process
  // so we know it'll still exist if we need to report
  // exception info and whatnot.
  Object::Ref<PythonContextCall> keep_alive_ref(this);

  PythonContextCall* prev_call = current_call_;
  current_call_ = this;
  assert(Python::HaveGIL());
  PyObject* o =
      PyObject_Call(object_.Get(),
                    args ? args
                         : g_core->python->objs()
                               .Get(core::CorePython::ObjID::kEmptyTuple)
                               .Get(),
                    nullptr);
  current_call_ = prev_call;

  if (o) {
    Py_DECREF(o);
  } else {
    // Save/restore python error or it can mess with context print calls.
    BA_PYTHON_ERROR_SAVE;

    PySys_WriteStderr("Exception in Python call:\n");
    PrintContext();
    BA_PYTHON_ERROR_RESTORE;

    // We pass zero here to avoid grabbing references to this exception
    // which can cause objects to stick around and trip up our deletion checks.
    // (nodes, actors existing after their games have ended).
    PyErr_PrintEx(0);
    PyErr_Clear();
  }
}

void PythonContextCall::PrintContext() {
  assert(g_base->InLogicThread());
  std::string s = std::string("  root call: ") + object().Str() + "\n";
  s += ("  root call origin: " + file_loc() + "\n");
  s += Python::GetContextBaseString();
  PySys_WriteStderr("%s\n", s.c_str());
}

void PythonContextCall::Schedule() {
  // Since we're mucking with Object::Refs, need to limit to logic thread.
  BA_PRECONDITION(g_base->InLogicThread());
  Object::Ref<PythonContextCall> ref(this);

  assert(base::g_base);
  schedule_count_++;
  base::g_base->logic->event_loop()->PushCall([ref] {
    assert(ref.Exists());
    ref->schedule_count_--;
    assert(ref->schedule_count_ >= 0);
    ref->Run();
  });
}
void PythonContextCall::ScheduleOnce() {
  if (schedule_count_ > 0) {
    return;
  }
  Schedule();
}

void PythonContextCall::Schedule(const PythonRef& args) {
  // Since we're mucking with Object::Refs, need to limit to logic thread.
  BA_PRECONDITION(g_base->InLogicThread());
  Object::Ref<PythonContextCall> ref(this);
  assert(base::g_base);
  schedule_count_++;
  base::g_base->logic->event_loop()->PushCall([ref, args] {
    assert(ref.Exists());
    ref->schedule_count_--;
    assert(ref->schedule_count_ >= 0);
    ref->Run(args);
  });
}
void PythonContextCall::ScheduleOnce(const PythonRef& args) {
  if (schedule_count_ > 0) {
    return;
  }
  Schedule(args);
}

void PythonContextCall::ScheduleWeak() {
  // Since we're mucking with Object::WeakRefs, need to limit to logic thread.
  BA_PRECONDITION(g_base->InLogicThread());
  Object::WeakRef<PythonContextCall> ref(this);
  assert(base::g_base);
  schedule_count_++;
  base::g_base->logic->event_loop()->PushCall([ref] {
    if (auto* call = ref.Get()) {
      call->schedule_count_--;
      assert(call->schedule_count_ >= 0);
      call->Run();
    }
  });
}

void PythonContextCall::ScheduleWeakOnce() {
  if (schedule_count_ > 0) {
    return;
  }
  ScheduleWeak();
}

void PythonContextCall::ScheduleWeak(const PythonRef& args) {
  // Since we're mucking with Object::WeakRefs, need to limit to logic thread.
  BA_PRECONDITION(g_base->InLogicThread());
  Object::WeakRef<PythonContextCall> ref(this);
  assert(base::g_base);
  schedule_count_++;
  base::g_base->logic->event_loop()->PushCall([ref, args] {
    if (auto* call = ref.Get()) {
      call->schedule_count_--;
      assert(call->schedule_count_ >= 0);
      call->Run(args);
    }
  });
}

void PythonContextCall::ScheduleWeakOnce(const PythonRef& args) {
  if (schedule_count_ > 0) {
    return;
  }
  ScheduleWeak(args);
}

}  // namespace ballistica::base
