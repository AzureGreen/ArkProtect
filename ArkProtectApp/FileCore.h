#pragma once


namespace ArkProtect
{
	class CFileCore
	{
	public:
		CFileCore(class CGlobal *GlobalObject);
		~CFileCore();

		BOOL DeleteFile(CString strFilePath);

		static DWORD CALLBACK DeleteFileCallback(LPARAM lParam);


	private:


		class CGlobal       *m_Global;
		static CFileCore    *m_File;

	};
}

