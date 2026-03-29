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

typedef struct _MMVAD
{
    MMVAD_SHORT Core;                                               
    struct _MMVAD_FLAGS2 VadFlags2;                                         
    struct _SUBSECTION* Subsection;                                         
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