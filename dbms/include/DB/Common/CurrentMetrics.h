#pragma once

#include <cstdint>
#include <utility>
#include <atomic>


/** Позволяет считать количество одновременно происходящих событий или текущее значение какой-либо метрики.
  *  - для высокоуровневого профайлинга.
  *
  * Также смотрите ProfileEvents.h
  * В ProfileEvents считается общее количество произошедших (точечных) событий - например, сколько раз были выполнены запросы.
  * В CurrentMetrics считается количество одновременных событий - например, сколько сейчас одновременно выполняется запросов,
  *  или текущее значение метрики - например, величина отставания реплики в секундах.
  */

#define APPLY_FOR_METRICS(M) \
	M(Query) \
	M(Merge) \
	M(ReplicatedFetch) \
	M(ReplicatedSend) \
	M(ReplicatedChecks) \
	M(BackgroundPoolTask) \
	M(DiskSpaceReservedForMerge) \
	M(DistributedSend) \
	M(QueryPreempted) \
	M(TCPConnection) \
	M(HTTPConnection) \
	M(InterserverConnection) \
	M(OpenFileForRead) \
	M(OpenFileForWrite) \
	M(Read) \
	M(Write) \
	M(SendExternalTables) \
	M(QueryThread) \
	M(ReadonlyReplica) \
	M(MemoryTracking) \
	M(MarkCacheBytes) \
	M(MarkCacheFiles) \
	M(UncompressedCacheBytes) \
	M(UncompressedCacheCells) \
	M(ReplicasMaxQueueSize) \
	M(ReplicasMaxInsertsInQueue) \
	M(ReplicasMaxMergesInQueue) \
	M(ReplicasSumQueueSize) \
	M(ReplicasSumInsertsInQueue) \
	M(ReplicasSumMergesInQueue) \
	M(ReplicasMaxAbsoluteDelay) \
	M(ReplicasMaxRelativeDelay) \
	M(MaxPartCountForPartition) \
	\
	M(END)

namespace CurrentMetrics
{
	/// Виды метрик.
	enum Metric
	{
	#define M(NAME) NAME,
		APPLY_FOR_METRICS(M)
	#undef M
	};


	/// Получить текстовое описание метрики по его enum-у.
	inline const char * getDescription(Metric event)
	{
		static const char * descriptions[] =
		{
		#define M(NAME) #NAME,
			APPLY_FOR_METRICS(M)
		#undef M
		};

		return descriptions[event];
	}


	using Value = int64_t;

	/// Счётчики - текущие значения метрик.
	extern std::atomic<Value> values[END];


	/// Выставить значение указанной метрики.
	inline void set(Metric metric, Value value)
	{
		values[metric] = value;
	}

	/// Прибавить величину к значению указанной метрики. Вы затем должны вычесть величину самостоятельно. Или см. ниже class Increment.
	inline void add(Metric metric, Value value = 1)
	{
		values[metric] += value;
	}

	inline void sub(Metric metric, Value value = 1)
	{
		add(metric, -value);
	}

	/// На время жизни объекта, увеличивает указанное значение на указанную величину.
	class Increment
	{
	private:
		std::atomic<Value> * what;
		Value amount;

		Increment(std::atomic<Value> * what, Value amount)
			: what(what), amount(amount)
		{
			*what += amount;
		}

	public:
		Increment(Metric metric, Value amount = 1)
			: Increment(&values[metric], amount) {}

		~Increment()
		{
			if (what)
				*what -= amount;
		}

		Increment(Increment && old)
		{
			*this = std::move(old);
		}

		Increment & operator= (Increment && old)
		{
			what = old.what;
			amount = old.amount;
			old.what = nullptr;
			return *this;
		}

		void changeTo(Value new_amount)
		{
			*what += new_amount - amount;
			amount = new_amount;
		}

		/// Уменьшить значение раньше вызова деструктора.
		void destroy()
		{
			*what -= amount;
			what = nullptr;
		}
	};
}


#undef APPLY_FOR_METRICS
