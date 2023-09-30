#pragma once
#include <mutex>
#include <queue>

namespace wc {
	template<typename T>
	class TSQueue {
	public:
		TSQueue()
			: m_m()
			, m_q()
		{

		}

		void push(T t) {
			std::lock_guard lock(m_m);
			m_q.push(t);
		}

		T pop() {
			std::lock_guard lock(m_m);
			T t = std::move(m_q.front());
			m_q.pop();
			return t;
		}

		bool empty() {
			std::lock_guard lock(m_m);
			return m_q.empty();
		}

		~TSQueue() { }
	private:
		std::mutex m_m;
		std::queue<T> m_q;
	};
}