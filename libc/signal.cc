#include "signal.hh"

namespace osv {

// we can't use have __thread sigset because of the constructor
__thread __attribute__((aligned(sizeof(sigset))))
    char thread_signal_mask[sizeof(sigset)];

struct sigaction signal_actions[nsignals];

sigset* from_libc(sigset_t* s)
{
    return reinterpret_cast<sigset*>(s);
}

const sigset* from_libc(const sigset_t* s)
{
    return reinterpret_cast<const sigset*>(s);
}

sigset* thread_signals()
{
    return reinterpret_cast<sigset*>(thread_signal_mask);
}

void generate_signal(siginfo_t &siginfo, exception_frame* ef)
{
    if (pthread_self() && thread_signals()->mask[siginfo.si_signo]) {
        // FIXME: need to find some other thread to deliver
        // FIXME: the signal to
        abort();
    }
    arch::build_signal_frame(ef, siginfo, signal_actions[siginfo.si_signo]);
}

void handle_segmentation_fault(ulong addr, exception_frame* ef)
{
    siginfo_t si;
    si.si_signo = SIGSEGV;
    si.si_addr = reinterpret_cast<void*>(addr);
    generate_signal(si, ef);
}

}

using namespace osv;

int sigemptyset(sigset_t* sigset)
{
    from_libc(sigset)->mask.reset();
    return 0;
}

int sigfillset(sigset_t *sigset)
{
    from_libc(sigset)->mask.set();
    return 0;
}

int sigaddset(sigset_t *sigset, int signum)
{
    from_libc(sigset)->mask.set(signum);
    return 0;
}

int sigprocmask(int how, const sigset_t* _set, sigset_t* _oldset)
{
    auto set = from_libc(_set);
    auto oldset = from_libc(_oldset);
    if (oldset) {
        *oldset = *thread_signals();
    }
    if (set) {
        switch (how) {
        case SIG_BLOCK:
            thread_signals()->mask |= set->mask;
            break;
        case SIG_UNBLOCK:
            thread_signals()->mask &= ~set->mask;
            break;
        case SIG_SETMASK:
            thread_signals()->mask = set->mask;
            break;
        default:
            abort();
        }
    }
    return 0;
}

int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact)
{
    if (oldact) {
        *oldact = signal_actions[signum];
    }
    if (act) {
        signal_actions[signum] = *act;
    }
    return 0;
}
