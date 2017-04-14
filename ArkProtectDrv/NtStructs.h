#ifndef CXX_NtStructs_H

#include <ntifs.h>

//////////////////////////////////////////////////////////////////////////
// Sectionœ‡πÿ

typedef struct _CONTROL_AREA
{
	struct _SEGMENT*	Segment;
	LIST_ENTRY			DereferenceList;
	UINT_PTR			NumberOfSectionReferences;
	UINT_PTR			NumberOfPfnReferences;
	UINT_PTR			NumberOfMappedViews;
	UINT_PTR			NumberOfUserReferences;
	UINT32				u;
	UINT32				FlushInProgressCount;
	PFILE_OBJECT		FilePointer;
	UINT32				ControlAreaLock;
	UINT32				ModifiedWriteCount;
	UINT_PTR			WaitingForDeletion;
	UINT_PTR			u2;
	UINT64				Padding;
	UINT_PTR			LockedPages;
	LIST_ENTRY			ViewList;
} CONTROL_AREA, *PCONTROL_AREA;


typedef struct _SEGMENT
{
	PCONTROL_AREA	ControlArea;
	UINT32			TotalNumberOfPtes;
	UINT32			SegmentFlags;
	UINT_PTR		NumberOfCommittedPages;
	UINT64			SizeOfSegment;
	union
	{
		UINT_PTR ExtendInfo;
		UINT_PTR BasedAddress;
	} u;
	UINT_PTR	SegmentLock;
	UINT_PTR	u1;
	UINT_PTR	u2;
	UINT_PTR	PrototypePte;
#ifndef _WIN64
	UINT32		Padding;
#endif // !_WIN64
	UINT_PTR	ThePtes;
} SEGMENT, *PSEGMENT;


typedef struct _SECTION_OBJECT
{
	PVOID	 StartingVa;
	PVOID	 EndingVa;
	PVOID	 Parent;
	PVOID	 LeftChild;
	PVOID	 RightChild;
	PSEGMENT Segment;
} SECTION_OBJECT, *PSECTION_OBJECT;



#endif // !CXX_NtStructs_H
