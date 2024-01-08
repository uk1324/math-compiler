#pragma once

#include "ints.hpp"

#define RESULT_DEBUG_CHECKS

#ifdef RESULT_DEBUG_CHECKS
#include "asserts.hpp"
#define RESULT_ASSERT(condition) ASSERT(condition)
#else
#define RESULT_ASSERT(condition)
#endif

#define TRY(result) \
	do { \
		if (result.isErr()) { \
			return result; \
		} \
	} while (false)

// TODO: make variable with name.
#define TRY_MAKE(variableName, result) \
	do { \
		if (result.isErr()) { \
			return result; \
		} \
	} while (false)

template<typename Err>
struct ResultErr {
	explicit ResultErr(Err&& err);

	Err err_;
};

enum class ResultState : u8 {
	OK, ERR
};

template<typename Ok, typename Err>
class [[nodiscard]] Result {
public:
	Result(const Ok& ok);
	Result(Ok&& ok) noexcept;
	Result(ResultErr<Err>&& err) noexcept;
	Result(const Result& other);
	Result(Result&& other) noexcept;
	~Result();

	Result& operator=(const Result& other);
	Result& operator=(Result&& other) noexcept;

	Result* operator->();
	const Result* operator->() const;
	// Use ok() instead of operator*

	bool operator==(const Result& other) const;
	bool operator!=(const Result& other) const;
	bool operator==(const Ok& value) const;
	bool operator!=(const Ok& value) const;

	Ok& ok();
	const Ok& ok() const;
	bool isOk() const;

	Err& err();
	const Err& err() const;
	bool isErr() const;

private:
	void destructUnion();

private:
	ResultState state;
	union {
		Ok ok_;
		Err err_;
	};
};

template<typename Ok, typename Err>
Result<Ok, Err>::Result(const Ok& ok)
	: ok_(ok)
	, state(ResultState::OK) {}

template<typename Ok, typename Err>
Result<Ok, Err>::Result(Ok&& ok) noexcept
	: ok_(std::move(ok))
	, state(ResultState::OK) {}

template<typename Ok, typename Err>
Result<Ok, Err>::Result(ResultErr<Err>&& err) noexcept
	: err_(std::move(err))
	, state(ResultState::ERR) {}

template<typename Ok, typename Err>
Result<Ok, Err>::Result(const Result<Ok, Err>& other) 
	: state(other.state) {
	switch (other.state) {

	case ResultState::OK:
		new (&ok_) Ok(other.ok_);
		break;

	case ResultState::ERR:
		new (&err_) Err(other.err_);
		break;

	}
}

template<typename Ok, typename Err>
Result<Ok, Err>::Result(Result<Ok, Err>&& other) noexcept
	: state(other.state) {
	switch (other.state) {

	case ResultState::OK:
		new (&ok_) Ok(std::move(other));
		break;

	case ResultState::ERR:
		new (&err_) Err(std::move(other));
		break;

	}
}

template<typename Ok, typename Err>
Result<Ok, Err>::~Result() {
	destructUnion();
}

template<typename Ok, typename Err>
Result<Ok, Err>& Result<Ok, Err>::operator=(const Result& other) {
	if (isOk()) {
		if (isOk()) {
			ok_ = other.ok_;
		} else {
			ok_.~Ok();
			new (this) Err(other.err_);
		}
	} else {
		if (isOk()) {
			err_.~Err();
			new (this) Ok(other.ok_);
		}
		else {
			err = other.err_;
		}
	}
	state = other.state;
}

template<typename Ok, typename Err>
Result<Ok, Err>& Result<Ok, Err>::operator=(Result&& other) noexcept {
	if (isOk()) {
		if (isOk()) {
			ok_ = std::move(other.ok_);
		}
		else {
			ok_.~Ok();
			new (this) Err(std::move(other.err_));
		}
	}
	else {
		if (isOk()) {
			err_.~Err();
			new (this) Ok(std::move(other.ok_));
		}
		else {
			err = std::move(other.err_);
		}
	}
	state = other.state;
}

template<typename Ok, typename Err>
Result<Ok, Err>* Result<Ok, Err>::operator->() {
	return &ok();
}

template<typename Ok, typename Err>
const Result<Ok, Err>* Result<Ok, Err>::operator->() const {
	return &ok();
}

template<typename Ok, typename Err>
bool Result<Ok, Err>::operator==(const Result& other) const {
	if (state != other.state) {
		return false;
	}

	switch (state) {
	case ResultState::OK: return ok_ == other.ok_;
	case ResultState::ERR: return err_ == other.err_;
	}
}

template<typename Ok, typename Err>
inline bool Result<Ok, Err>::operator!=(const Result& other) const {
	return !(*this == other);
}

template<typename Ok, typename Err>
bool Result<Ok, Err>::operator==(const Ok& value) const {
	if (isOk()) {
		return ok_ == value;
	}
	return false;
}

template<typename Ok, typename Err>
bool Result<Ok, Err>::operator!=(const Ok& value) const {
	return !(*this == value);
}

template<typename Ok, typename Err>
Ok& Result<Ok, Err>::ok() {
	RESULT_ASSERT(isOk());
	return ok_;
}

template<typename Ok, typename Err>
const Ok& Result<Ok, Err>::ok() const {
	return const_cast<Result*>(this)->ok();
}

template<typename Ok, typename Err>
bool Result<Ok, Err>::isOk() const {
	return state == ResultState::OK;
}

template<typename Ok, typename Err>
Err& Result<Ok, Err>::err() {
	ASSERT(isErr());
	return err_;
}

template<typename Ok, typename Err>
const Err& Result<Ok, Err>::err() const {
	return const_cast<Result*>(this)->err();
}

template<typename Ok, typename Err>
bool Result<Ok, Err>::isErr() const {
	return state == ResultState::ERR;
}

template<typename Ok, typename Err>
void Result<Ok, Err>::destructUnion() {
	switch (state) {

	case ResultState::OK:
		ok_.~Ok();
		break;

	case ResultState::ERR:
		err_.~Err();
		break;
	}
}

template<typename Err>
ResultErr<Err>::ResultErr(Err&& err)
	: err_(err) {}

#undef RESULT_ASSERT