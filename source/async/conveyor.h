#pragma once

#include <cstdint>

#include "mem.h"

namespace dvr {
	typedef uint64_t ConveyorDescriptor;

	class ConveyorSignal {
	private:
		int64_t code;
		std::string message;
	public:
	};

	class ConveyorSignalOrValue {
	public:
		virtual ~ConveyorSignalOrValue() = default;
	};

	template<typename T>
	class ConveyorSignalOr : public ConveyorSignalOrValue {
	public:
	};

	class ConveyorNode {
	public:
		virtual ~ConveyorNode() = default;
	};

	class ConveyorBase {
	private:
		ConveyorDescriptor desc;
	public:
		enum class Type {
			IN,
			OUT
		};

		virtual ~ConveyorBase() = default;

		ConveyorDescriptor descriptor() const;
		virtual Type type() const = 0;
	};

	template<typename T>
	class Conveyor : public ConveyorBase {
	private:
		Our<ConveyorNode> node;
	public:
		virtual ~Conveyor() = default;

		virtual T wait() = 0;

		/*
		 * Func is basically std::function<T2(T&&)>
		 * Because Conveyor<void> fail this is mitgated by
		 * specifying Func.
		 */
		template<typename T2, typename Func>
		Conveyor<T2> then(Func&& func);
		
		template<class... Attachments>
		void attach(Attachments&&... attachements);
	};

	template<typename T>
	class ConveyorFeeder{
	private:
	public:
		virtual ~ConveyorFeeder() = default;

		virtual void feed(T&& ) = 0;
		virtual void signal(ConveyorSignal&& signal) = 0;
	};

	template<typename T>
	struct ConveyorSystem {
		Own<ConveyorFeeder<T>> input;
		Conveyor<T> output;
	};

	template<typename T>
	ConveyorSystem<T> makeConveyorSystem();
}
