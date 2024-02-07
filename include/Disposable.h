#pragma once

#ifndef DISPOSABLE_H
#define DISPOSABLE_H

namespace VS3CODEFACTORY::CORE
{
	class Disposable
	{
	protected:
		bool m_disposed;

		Disposable()
			: m_disposed(false) {}

	public:
		void Dispose()
		{
			if (!m_disposed)
			{
				DisposeManagedObjects();
				m_disposed = true;
			}
		}

		virtual void DisposeManagedObjects() {}
	};
}

#endif // DISPOSABLE_H