#pragma once
#pragma warning(disable: 4201)

typedef struct _RTL_AVL_TREE
{
    struct _RTL_BALANCED_NODE* Root;
} RTL_AVL_TREE, * PRTL_AVL_TREE;

typedef struct _MMVAD_FLAGS
{
    union
    {
        struct
        {
            ULONG Lock : 1;
            ULONG LockContended : 1;
            ULONG DeleteInProgress : 1;
            ULONG NoChange : 1;
            ULONG VadType : 3;
            ULONG Protection : 5;
            ULONG PreferredNode : 7;
            ULONG PageSize : 2;
            ULONG PrivateMemory : 1;
        };
        ULONG EntireField;
    };
} MMVAD_FLAGS, * PMMVAD_FLAGS;

typedef struct _MMVAD_FLAGS2
{
    union
    {
        struct
        {
            ULONG Large : 1;                                                  
            ULONG TrimBehind : 1;                                             
            ULONG Inherit : 1;                                                
            ULONG NoValidationNeeded : 1;                                     
            ULONG PrivateDemandZero : 1;                                      
            ULONG ImageMappingExtended : 1;                                   
            ULONG Spare : 26;                                                 
        };
        ULONG LongFlags;                                                    
    };
} MMVAD_FLAGS2, * PMMVAD_FLAGS2;

typedef struct _MM_PRIVATE_VAD_FLAGS
{
    ULONG Lock : 1;
    ULONG LockContended : 1;
    ULONG DeleteInProgress : 1;
    ULONG NoChange : 1;
    ULONG VadType : 3;
    ULONG Protection : 5;
    ULONG PreferredNode : 7;
    ULONG PageSize : 2;
    ULONG PrivateMemoryAlwaysSet : 1;
    ULONG WriteWatch : 1;
    ULONG FixedLargePageSize : 1;
    ULONG ZeroFillPagesOptional : 1;
    ULONG MemCommit : 1;
    ULONG Graphics : 1;
    ULONG Enclave : 1;
    ULONG ShadowStack : 1;
    ULONG PhysicalMemoryPfnsReferenced : 1;
} MM_PRIVATE_VAD_FLAGS, * PMM_PRIVATE_VAD_FLAGS;

typedef struct _MM_SHARED_VAD_FLAGS
{
    ULONG Lock : 1;
    ULONG LockContended : 1;
    ULONG DeleteInProgress : 1;
    ULONG NoChange : 1;
    ULONG VadType : 3;
    ULONG Protection : 5;
    ULONG PreferredNode : 7;
    ULONG PageSize : 2;
    ULONG PrivateMemoryAlwaysClear : 1;
    ULONG PrivateFixup : 1;
    ULONG HotPatchState : 2;
} MM_SHARED_VAD_FLAGS, * PMM_SHARED_VAD_FLAGS;


typedef ULONG_PTR _EX_PUSH_LOCK;
typedef struct _MI_VAD_EVENT_BLOCK MI_VAD_EVENT_BLOCK, * PMI_VAD_EVENT_BLOCK;

typedef struct _MMVAD_SHORT
{
    union
    {
        struct
        {
            struct _MMVAD_SHORT* NextVad;
            VOID* ExtraCreateInfo;
        };
        struct _RTL_BALANCED_NODE VadNode;
    };
    ULONG StartingVpn;
    ULONG EndingVpn;
    UCHAR StartingVpnHigh;
    UCHAR EndingVpnHigh;
    UCHAR CommitChargeHigh;
    UCHAR SpareNT64VadUChar;
    volatile LONG ReferenceCount;
    _EX_PUSH_LOCK PushLock;
    union
    {
        ULONG LongFlags;
        MMVAD_FLAGS VadFlags;
        MM_PRIVATE_VAD_FLAGS PrivateVadFlags;
        MM_SHARED_VAD_FLAGS SharedVadFlags;
        volatile ULONG VolatileVadLong;
    } u;
    ULONG CommitCharge;
    union
    {
        struct _MI_VAD_EVENT_BLOCK* EventList;
    } u5;
}MMVAD_SHORT, * PMMVAD_SHORT;

typedef struct _MI_VAD_SEQUENTIAL_INFO
{
    union
    {
        struct
        {
            ULONGLONG Length : 12;                                            
            ULONGLONG Vpn : 51;                                               
            ULONGLONG MustBeZero : 1;                                         
        };
        ULONGLONG EntireField;                                              
    };
} MI_VAD_SEQUENTIAL_INFO, * PMI_VAD_SEQUENTIAL_INFO;


//0x4 bytes (sizeof)
struct _MMSECTION_FLAGS
{
    ULONG BeingDeleted : 1;                                                   //0x0
    ULONG BeingCreated : 1;                                                   //0x0
    ULONG BeingPurged : 1;                                                    //0x0
    ULONG NoModifiedWriting : 1;                                              //0x0
    ULONG FailAllIo : 1;                                                      //0x0
    ULONG Image : 1;                                                          //0x0
    ULONG Based : 1;                                                          //0x0
    ULONG File : 1;                                                           //0x0
    ULONG SectionOfInterest : 1;                                              //0x0
    ULONG PrefetchCreated : 1;                                                //0x0
    ULONG PhysicalMemory : 1;                                                 //0x0
    ULONG ImageControlAreaOnRemovableMedia : 1;                               //0x0
    ULONG Reserve : 1;                                                        //0x0
    ULONG Commit : 1;                                                         //0x0
    ULONG NoChange : 1;                                                       //0x0
    ULONG WasPurged : 1;                                                      //0x0
    ULONG UserReference : 1;                                                  //0x0
    ULONG GlobalMemory : 1;                                                   //0x0
    ULONG DeleteOnClose : 1;                                                  //0x0
    ULONG FilePointerNull : 1;                                                //0x0
    ULONG PreferredNode : 7;                                                  //0x0
    ULONG GlobalOnlyPerSession : 1;                                           //0x0
    ULONG ControlAreaOnUnusedList : 1;                                        //0x0
    ULONG SystemVaAllocated : 1;                                              //0x0
    ULONG NotBeingUsed : 1;                                                   //0x0
    ULONG PageSize64K : 1;                                                    //0x0
};

//0x8 bytes (sizeof)
struct _EX_FAST_REF
{
    union
    {
        VOID* Object;                                                       //0x0
        ULONGLONG RefCnt : 4;                                                 //0x0
        ULONGLONG Value;                                                    //0x0
    };
};

typedef struct _CONTROL_AREA
{
    struct _SEGMENT* Segment;                                               //0x0
    union
    {
        struct _LIST_ENTRY ListHead;                                        //0x8
        VOID* AweContext;                                                   //0x8
    };
    ULONGLONG NumberOfSectionReferences;                                    //0x18
    ULONGLONG NumberOfPfnReferences;                                        //0x20
    ULONGLONG NumberOfMappedViews;                                          //0x28
    ULONGLONG NumberOfUserReferences;                                       //0x30
    union
    {
        ULONG LongFlags;                                                    //0x38
        struct _MMSECTION_FLAGS Flags;                                      //0x38
    } u;                                                                    //0x38
    union
    {
        ULONG LongFlags;                                                    //0x3c
        ULONG Flags;                                                        //0x3c
    } u1;                                                                   //0x3c
    struct _EX_FAST_REF FilePointer;                                        //0x40
    volatile LONG ControlAreaLock;                                          //0x48
    ULONG ModifiedWriteCount;                                               //0x4c
    struct _MI_CONTROL_AREA_WAIT_BLOCK* WaitList;                           //0x50
    union
    {
        struct
        {
            union
            {
                ULONG NumberOfSystemCacheViews;                             //0x58
                ULONG ImageRelocationStartBit;                              //0x58
            };
            union
            {
                volatile LONG WritableUserReferences;                       //0x5c
                struct
                {
                    ULONG ImageRelocationSizeIn64k : 16;                      //0x5c
                    ULONG SystemImage : 1;                                    //0x5c
                    ULONG CantMove : 1;                                       //0x5c
                    ULONG StrongCode : 2;                                     //0x5c
                    ULONG BitMap : 2;                                         //0x5c
                    ULONG ImageActive : 1;                                    //0x5c
                    ULONG ImageBaseOkToReuse : 1;                             //0x5c
                };
            };
            union
            {
                ULONG NumberOfSubsections;                                  //0x60
                PVOID ImageInfoRef;                      //0x60
            };
        } e2;                                                               //0x58
    } u2;                                                                   //0x58
    PVOID FileObjectLock;                                    //0x68
    volatile ULONGLONG LockedPages;                                         //0x70
    union
    {
        ULONGLONG Spare : 3;                                                  //0x78
        ULONGLONG IoAttributionContext : 61;                                  //0x78
        ULONGLONG ImageCrossPartitionCharge;                                //0x78
        ULONGLONG CommittedPageCount : 36;                                    //0x78
    } u3;                                                                   //0x78
} CONTROL_AREA, *PCONTROL_AREA;

typedef struct _SUBSECTION
{
    PCONTROL_AREA ControlArea;
} SUBSECTION, *PSUBSECTION;

typedef struct _MMVAD
{
    MMVAD_SHORT Core;                                               
    struct _MMVAD_FLAGS2 VadFlags2;                                         
    PSUBSECTION Subsection;
    struct _MMPTE* FirstPrototypePte;                                       
    struct _MMPTE* LastContiguousPte;                                       
    struct _LIST_ENTRY ViewLinks;                                           
    struct _EPROCESS* VadsProcess;                                          
    union
    {
        MI_VAD_SEQUENTIAL_INFO SequentialVa;                        
        struct _MMEXTEND_INFO* ExtendedInfo;                                
    } u4;                                                                   
    struct _FILE_OBJECT* FileObject;                                        
} MMVAD, * PMMVAD;


// VAD Protection Values:
// 0 = MM_ZERO_ACCESS
// 1 = MM_READONLY
// 2 = MM_EXECUTE
// 3 = MM_EXECUTE_READ
// 4 = MM_READWRITE
// 5 = MM_WRITECOPY
// 6 = MM_EXECUTE_READWRITE
// 7 = MM_EXECUTE_WRITECOPY

typedef enum _MM_PROTECTION
{
    MM_ZERO_ACCESS = 0,
    MM_READONLY = 1,
    MM_EXECUTE = 2,
    MM_EXECUTE_READ = 3,
    MM_READWRITE = 4,
    MM_WRITECOPY = 5,
    MM_EXECUTE_READWRITE = 6,
    MM_EXECUTE_WRITECOPY = 7
} MM_PROTECTION;



//temporary PTE defs

#pragma warning(disable: 4201)

typedef struct _PML4E
{
    union
    {
        struct
        {
            ULONG64 Present : 1;              // Must be 1, region invalid if 0.
            ULONG64 ReadWrite : 1;            // If 0, writes not allowed.
            ULONG64 UserSupervisor : 1;       // If 0, user-mode accesses not allowed.
            ULONG64 PageWriteThrough : 1;     // Determines the memory type used to access PDPT.
            ULONG64 PageCacheDisable : 1;     // Determines the memory type used to access PDPT.
            ULONG64 Accessed : 1;             // If 0, this entry has not been used for translation.
            ULONG64 Ignored1 : 1;
            ULONG64 PageSize : 1;             // Must be 0 for PML4E.
            ULONG64 Ignored2 : 4;
            ULONG64 PageFrameNumber : 36;     // The page frame number of the PDPT of this PML4E.
            ULONG64 Reserved : 4;
            ULONG64 Ignored3 : 11;
            ULONG64 ExecuteDisable : 1;       // If 1, instruction fetches not allowed.
        };
        ULONG64 Value;
    };
} PML4E, * PPML4E, PDPT, * PPDPT, PDE, * PPDE, PTE, * PPTE;