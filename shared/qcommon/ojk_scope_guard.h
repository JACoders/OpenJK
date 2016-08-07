//
// Calls specified callbacks on scope enter (optional) and exit.
//


#ifndef OJK_SCOPE_GUARD_INCLUDED
#define OJK_SCOPE_GUARD_INCLUDED


#include <functional>
#include <utility>


namespace ojk
{


class ScopeGuard
{
public:
	using Callback = std::function<void()>;


	ScopeGuard(
		Callback leave_callback) :
			leave_callback_(leave_callback)
	{
	}

	ScopeGuard(
		Callback enter_callback,
		Callback leave_callback) :
		ScopeGuard(leave_callback)
	{
		enter_callback();
	}

	ScopeGuard(
		const ScopeGuard& that) = delete;

	ScopeGuard& operator=(
		const ScopeGuard& that) = delete;

	~ScopeGuard()
	{
		leave_callback_();
	}


private:
	Callback leave_callback_;
}; // ScopeGuard


} // ojk


#endif // OJK_SCOPE_GUARD_INCLUDED
