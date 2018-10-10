// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <utility>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/scheduler.h"

namespace Kernel {

std::mutex Scheduler::scheduler_mutex;

Scheduler::Scheduler(Core::ARM_Interface& cpu_core) : cpu_core(cpu_core) {}

Scheduler::~Scheduler() {
    for (auto& thread : thread_list) {
        thread->Stop();
    }
}

bool Scheduler::HaveReadyThreads() const {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    return ready_queue.get_first() != nullptr;
}

Thread* Scheduler::GetCurrentThread() const {
    return current_thread.get();
}

Thread* Scheduler::PopNextReadyThread() {
    Thread* next = nullptr;
    Thread* thread = GetCurrentThread();

    if (thread && thread->GetStatus() == ThreadStatus::Running) {
        // We have to do better than the current thread.
        // This call returns null when that's not possible.
        next = ready_queue.pop_first_better(thread->GetPriority());
        if (!next) {
            // Otherwise just keep going with the current thread
            next = thread;
        }
    } else {
        next = ready_queue.pop_first();
    }

    return next;
}

void Scheduler::SwitchContext(Thread* new_thread) {
    Thread* previous_thread = GetCurrentThread();

    // Save context for previous thread
    if (previous_thread) {
        cpu_core.SaveContext(previous_thread->GetContext());
        // Save the TPIDR_EL0 system register in case it was modified.
        previous_thread->SetTPIDR_EL0(cpu_core.GetTPIDR_EL0());

        if (previous_thread->GetStatus() == ThreadStatus::Running) {
            // This is only the case when a reschedule is triggered without the current thread
            // yielding execution (i.e. an event triggered, system core time-sliced, etc)
            ready_queue.push_front(previous_thread->GetPriority(), previous_thread);
            previous_thread->SetStatus(ThreadStatus::Ready);
        }
    }

    // Load context of new thread
    if (new_thread) {
        ASSERT_MSG(new_thread->GetStatus() == ThreadStatus::Ready,
                   "Thread must be ready to become running.");

        // Cancel any outstanding wakeup events for this thread
        new_thread->CancelWakeupTimer();

        auto* const previous_process = Core::CurrentProcess();

        current_thread = new_thread;

        ready_queue.remove(new_thread->GetPriority(), new_thread);
        new_thread->SetStatus(ThreadStatus::Running);

        auto* const thread_owner_process = current_thread->GetOwnerProcess();
        if (previous_process != thread_owner_process) {
            Core::System::GetInstance().Kernel().MakeCurrentProcess(thread_owner_process);
            SetCurrentPageTable(&Core::CurrentProcess()->VMManager().page_table);
        }

        cpu_core.LoadContext(new_thread->GetContext());
        cpu_core.SetTlsAddress(new_thread->GetTLSAddress());
        cpu_core.SetTPIDR_EL0(new_thread->GetTPIDR_EL0());
        cpu_core.ClearExclusiveState();
    } else {
        current_thread = nullptr;
        // Note: We do not reset the current process and current page table when idling because
        // technically we haven't changed processes, our threads are just paused.
    }
}

void Scheduler::Reschedule() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);

    Thread* cur = GetCurrentThread();
    Thread* next = PopNextReadyThread();

    if (cur && next) {
        LOG_TRACE(Kernel, "context switch {} -> {}", cur->GetObjectId(), next->GetObjectId());
    } else if (cur) {
        LOG_TRACE(Kernel, "context switch {} -> idle", cur->GetObjectId());
    } else if (next) {
        LOG_TRACE(Kernel, "context switch idle -> {}", next->GetObjectId());
    }

    SwitchContext(next);
}

void Scheduler::AddThread(SharedPtr<Thread> thread, u32 priority) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);

    thread_list.push_back(std::move(thread));
    ready_queue.prepare(priority);
}

void Scheduler::RemoveThread(Thread* thread) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);

    thread_list.erase(std::remove(thread_list.begin(), thread_list.end(), thread),
                      thread_list.end());
}

void Scheduler::ScheduleThread(Thread* thread, u32 priority) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);

    ASSERT(thread->GetStatus() == ThreadStatus::Ready);
    ready_queue.push_back(priority, thread);
}

void Scheduler::UnscheduleThread(Thread* thread, u32 priority) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);

    ASSERT(thread->GetStatus() == ThreadStatus::Ready);
    ready_queue.remove(priority, thread);
}

void Scheduler::SetThreadPriority(Thread* thread, u32 priority) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);

    // If thread was ready, adjust queues
    if (thread->GetStatus() == ThreadStatus::Ready)
        ready_queue.move(thread, thread->GetPriority(), priority);
    else
        ready_queue.prepare(priority);
}

} // namespace Kernel
