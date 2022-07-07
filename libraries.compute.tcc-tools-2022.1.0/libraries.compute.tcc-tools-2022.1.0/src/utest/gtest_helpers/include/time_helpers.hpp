/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef PTP_HELPERS_HPP
#define PTP_HELPERS_HPP

#include <linux/ptp_clock.h>
#include <type_traits>
#include <stdint.h>
#include <time.h>
#include <iostream>

/* Helper functions, operators etc for time structures
 * (timespect, ptp_clock_time) to simplify tests
 */

#define BILLION 1000000000UL

// Helper structure
template <class T>
struct ts_wrapper_base {
protected:
	ts_wrapper_base(T t) : _t(t)
	{
		static_assert (std::is_same<T, timespec>::value ||
			       std::is_same<T, ptp_clock_time>::value,
			       "T must be timespec or ptp_clock_time");
	}
protected:
	T _t;
};

// Wrapper to have similar interface for ptp_clock_time and timespec
template <class T>
class ts_wrapper { };

template<>
class ts_wrapper<timespec> : ts_wrapper_base<timespec>
{
public:
	ts_wrapper(timespec t) : ts_wrapper_base(t) {}
	time_t sec() { return _t.tv_sec; }
	__syscall_slong_t nsec() { return _t.tv_nsec; }
};

template<>
class ts_wrapper<ptp_clock_time> : ts_wrapper_base<ptp_clock_time>
{
public:
	ts_wrapper(ptp_clock_time t) : ts_wrapper_base(t) {}
	__s64 sec() { return _t.sec; }
	__u32 nsec() { return _t.nsec; }
};

// Converts struct (sec, nsec) to the nsec
uint64_t ts2ns(time_t sec, long nsec)
{
	return sec * BILLION + nsec;
}
template <class T>
uint64_t ts2ns(const T& ts)
{
	ts_wrapper<T> wts(ts);
	return wts.sec() * BILLION + wts.nsec();
}

// Converts struct (sec, nsec) to the seconds
template<class T>
double ts2s(const T& ts)
{
	return ts2ns(ts) / (double)BILLION;
}

// Converts nanoseconds to struct (sec, nsec)
template<class T>
T ns2time(const uint64_t ns)
{
	return T{ns / BILLION, ns % BILLION};
}

// Converts timespec <-> ptp_clock_time
ptp_clock_time ts2ptp(const timespec& ts)
{
	return {ts.tv_sec, static_cast<uint32_t>(ts.tv_nsec)};
}
timespec ptp2ts(const ptp_clock_time& time)
{
	return {time.sec, time.nsec};
}

// Set nanoseconds in timestamp to zero
uint64_t set_ns_to_zero(uint64_t ns)
{
	return ns / BILLION * BILLION;
}

// Satisfy requirement that BILLION in  [0, 999999999]
template<class T>
T normalize_ts(const T&  ts)
{
	ts_wrapper<T> wts(ts);
	return { static_cast<time_t>(wts.sec() + wts.nsec() / BILLION), static_cast<long>(wts.nsec() % BILLION) };
}

// timespec or ptp_clock_time arithmetic
template<class T>
T operator+(const T& a, const T& b)
{
	ts_wrapper<T> wa(a), wb(b);
	T t{wa.sec() + wb.sec(), wa.nsec() + wb.nsec()};
	return normalize_ts(t);
}

template<class T>
T operator-(const T& a, const T& b)
{
	ts_wrapper<T> wa(a), wb(b);
	T t{wa.sec() - wb.sec(), wa.nsec() - wb.nsec()};
	return normalize_ts(t);
}

template<class T, class K>
T operator*(const T& a, const K b)
{
	ts_wrapper<T> wa(a);
	T t{wa.sec() * b, wa.nsec() * b};
	return normalize_ts(t);
}

template<class T, class K>
T operator/(const T& a, const K b)
{
	ts_wrapper<T> wa(a);
	T t{wa.sec() / b, wa.nsec() / b};
	return normalize_ts(t);
}

// timespec or ptp_clock_time comparison
template<class T>
bool operator==(const T& a, const T& b)
{
	ts_wrapper<T> wa(a), wb(b);
	return wa.sec() == wb.sec() &&
	       wa.nsec() == wb.nsec();
}

template<typename T>
std::ostream& print_to_stream(std::ostream &stream, const T& a)
{
	ts_wrapper<T> wa(a);
	stream << "{\"sec\":" << wa.sec() <<",\"nsec\":" << wa.nsec() << "}";
	return stream;
}

std::ostream& operator <<(std::ostream &stream, const struct timespec& a)
{
	return print_to_stream(stream, a);
}

std::ostream& operator <<(std::ostream &stream, const struct ptp_clock_time& a)
{
	return print_to_stream(stream, a);
}
#endif // PTP_HELPERS_HPP
