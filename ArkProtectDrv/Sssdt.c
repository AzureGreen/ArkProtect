#include "Sssdt.h"

extern DYNAMIC_DATA            g_DynamicData;
extern PLDR_DATA_TABLE_ENTRY   g_PsLoadedModuleList;

PVOID    g_ReloadWin32kImage = NULL;       // 重载Win32k的基地址
PKSERVICE_TABLE_DESCRIPTOR g_CurrentWin32pServiceTableAddress = NULL;   // 当前系统运行着的Win32k的ServiceTable基地址
KSERVICE_TABLE_DESCRIPTOR  g_ReloadWin32pServiceTableAddress = { 0 };   // ShadowSsdt也在Ntkrnl里，ShawdowSsdt->base在Win32k里
UINT_PTR g_OriginalSssdtFunctionAddress[0x400] = { 0 };    // SssdtFunction原本的地址
//UINT32   g_SssdtItem[0x400] = { 0 };                       // Sssdt表里面原始存放的数据

PWCHAR g_SssdtFunctionName[0x400] =
{
	L"NtUserGetThreadState"
	,L"NtUserPeekMessage"
	,L"NtUserCallOneParam"
	,L"NtUserGetKeyState"
	,L"NtUserInvalidateRect"
	,L"NtUserCallNoParam"
	,L"NtUserGetMessage"
	,L"NtUserMessageCall"
	,L"NtGdiBitBlt"
	,L"NtGdiGetCharSet"
	,L"NtUserGetDC"
	,L"NtGdiSelectBitmap"
	,L"NtUserWaitMessage"
	,L"NtUserTranslateMessage"
	,L"NtUserGetProp"
	,L"NtUserPostMessage"
	,L"NtUserQueryWindow"
	,L"NtUserTranslateAccelerator"
	,L"NtGdiFlush"
	,L"NtUserRedrawWindow"
	,L"NtUserWindowFromPoint"
	,L"NtUserCallMsgFilter"
	,L"NtUserValidateTimerCallback"
	,L"NtUserBeginPaint"
	,L"NtUserSetTimer"
	,L"NtUserEndPaint"
	,L"NtUserSetCursor"
	,L"NtUserKillTimer"
	,L"NtUserBuildHwndList"
	,L"NtUserSelectPalette"
	,L"NtUserCallNextHookEx"
	,L"NtUserHideCaret"
	,L"NtGdiIntersectClipRect"
	,L"NtUserCallHwndLock"
	,L"NtUserGetProcessWindowStation"
	,L"NtGdiDeleteObjectApp"
	,L"NtUserSetWindowPos"
	,L"NtUserShowCaret"
	,L"NtUserEndDeferWindowPosEx"
	,L"NtUserCallHwndParamLock"
	,L"NtUserVkKeyScanEx"
	,L"NtGdiSetDIBitsToDeviceInternal"
	,L"NtUserCallTwoParam"
	,L"NtGdiGetRandomRgn"
	,L"NtUserCopyAcceleratorTable"
	,L"NtUserNotifyWinEvent"
	,L"NtGdiExtSelectClipRgn"
	,L"NtUserIsClipboardFormatAvailable"
	,L"NtUserSetScrollInfo"
	,L"NtGdiStretchBlt"
	,L"NtUserCreateCaret"
	,L"NtGdiRectVisible"
	,L"NtGdiCombineRgn"
	,L"NtGdiGetDCObject"
	,L"NtUserDispatchMessage"
	,L"NtUserRegisterWindowMessage"
	,L"NtGdiExtTextOutW"
	,L"NtGdiSelectFont"
	,L"NtGdiRestoreDC"
	,L"NtGdiSaveDC"
	,L"NtUserGetForegroundWindow"
	,L"NtUserShowScrollBar"
	,L"NtUserFindExistingCursorIcon"
	,L"NtGdiGetDCDword"
	,L"NtGdiGetRegionData"
	,L"NtGdiLineTo"
	,L"NtUserSystemParametersInfo"
	,L"NtGdiGetAppClipBox"
	,L"NtUserGetAsyncKeyState"
	,L"NtUserGetCPD"
	,L"NtUserRemoveProp"
	,L"NtGdiDoPalette"
	,L"NtGdiPolyPolyDraw"
	,L"NtUserSetCapture"
	,L"NtUserEnumDisplayMonitors"
	,L"NtGdiCreateCompatibleBitmap"
	,L"NtUserSetProp"
	,L"NtGdiGetTextCharsetInfo"
	,L"NtUserSBGetParms"
	,L"NtUserGetIconInfo"
	,L"NtUserExcludeUpdateRgn"
	,L"NtUserSetFocus"
	,L"NtGdiExtGetObjectW"
	,L"NtUserDeferWindowPos"
	,L"NtUserGetUpdateRect"
	,L"NtGdiCreateCompatibleDC"
	,L"NtUserGetClipboardSequenceNumber"
	,L"NtGdiCreatePen"
	,L"NtUserShowWindow"
	,L"NtUserGetKeyboardLayoutList"
	,L"NtGdiPatBlt"
	,L"NtUserMapVirtualKeyEx"
	,L"NtUserSetWindowLong"
	,L"NtGdiHfontCreate"
	,L"NtUserMoveWindow"
	,L"NtUserPostThreadMessage"
	,L"NtUserDrawIconEx"
	,L"NtUserGetSystemMenu"
	,L"NtGdiDrawStream"
	,L"NtUserInternalGetWindowText"
	,L"NtUserGetWindowDC"
	,L"NtGdiD3dDrawPrimitives2"
	,L"NtGdiInvertRgn"
	,L"NtGdiGetRgnBox"
	,L"NtGdiGetAndSetDCDword"
	,L"NtGdiMaskBlt"
	,L"NtGdiGetWidthTable"
	,L"NtUserScrollDC"
	,L"NtUserGetObjectInformation"
	,L"NtGdiCreateBitmap"
	,L"NtUserFindWindowEx"
	,L"NtGdiPolyPatBlt"
	,L"NtUserUnhookWindowsHookEx"
	,L"NtGdiGetNearestColor"
	,L"NtGdiTransformPoints"
	,L"NtGdiGetDCPoint"
	,L"NtGdiCreateDIBBrush"
	,L"NtGdiGetTextMetricsW"
	,L"NtUserCreateWindowEx"
	,L"NtUserSetParent"
	,L"NtUserGetKeyboardState"
	,L"NtUserToUnicodeEx"
	,L"NtUserGetControlBrush"
	,L"NtUserGetClassName"
	,L"NtGdiAlphaBlend"
	,L"NtGdiDdBlt"
	,L"NtGdiOffsetRgn"
	,L"NtUserDefSetText"
	,L"NtGdiGetTextFaceW"
	,L"NtGdiStretchDIBitsInternal"
	,L"NtUserSendInput"
	,L"NtUserGetThreadDesktop"
	,L"NtGdiCreateRectRgn"
	,L"NtGdiGetDIBitsInternal"
	,L"NtUserGetUpdateRgn"
	,L"NtGdiDeleteClientObj"
	,L"NtUserGetIconSize"
	,L"NtUserFillWindow"
	,L"NtGdiExtCreateRegion"
	,L"NtGdiComputeXformCoefficients"
	,L"NtUserSetWindowsHookEx"
	,L"NtUserNotifyProcessCreate"
	,L"NtGdiUnrealizeObject"
	,L"NtUserGetTitleBarInfo"
	,L"NtGdiRectangle"
	,L"NtUserSetThreadDesktop"
	,L"NtUserGetDCEx"
	,L"NtUserGetScrollBarInfo"
	,L"NtGdiGetTextExtent"
	,L"NtUserSetWindowFNID"
	,L"NtGdiSetLayout"
	,L"NtUserCalcMenuBar"
	,L"NtUserThunkedMenuItemInfo"
	,L"NtGdiExcludeClipRect"
	,L"NtGdiCreateDIBSection"
	,L"NtGdiGetDCforBitmap"
	,L"NtUserDestroyCursor"
	,L"NtUserDestroyWindow"
	,L"NtUserCallHwndParam"
	,L"NtGdiCreateDIBitmapInternal"
	,L"NtUserOpenWindowStation"
	,L"NtGdiDdDeleteSurfaceObject"
	,L"NtGdiDdCanCreateSurface"
	,L"NtGdiDdCreateSurface"
	,L"NtUserSetCursorIconData"
	,L"NtGdiDdDestroySurface"
	,L"NtUserCloseDesktop"
	,L"NtUserOpenDesktop"
	,L"NtUserSetProcessWindowStation"
	,L"NtUserGetAtomName"
	,L"NtGdiDdResetVisrgn"
	,L"NtGdiExtCreatePen"
	,L"NtGdiCreatePaletteInternal"
	,L"NtGdiSetBrushOrg"
	,L"NtUserBuildNameList"
	,L"NtGdiSetPixel"
	,L"NtUserRegisterClassExWOW"
	,L"NtGdiCreatePatternBrushInternal"
	,L"NtUserGetAncestor"
	,L"NtGdiGetOutlineTextMetricsInternalW"
	,L"NtGdiSetBitmapBits"
	,L"NtUserCloseWindowStation"
	,L"NtUserGetDoubleClickTime"
	,L"NtUserEnableScrollBar"
	,L"NtGdiCreateSolidBrush"
	,L"NtUserGetClassInfoEx"
	,L"NtGdiCreateClientObj"
	,L"NtUserUnregisterClass"
	,L"NtUserDeleteMenu"
	,L"NtGdiRectInRegion"
	,L"NtUserScrollWindowEx"
	,L"NtGdiGetPixel"
	,L"NtUserSetClassLong"
	,L"NtUserGetMenuBarInfo"
	,L"NtGdiDdCreateSurfaceEx"
	,L"NtGdiDdCreateSurfaceObject"
	,L"NtGdiGetNearestPaletteIndex"
	,L"NtGdiDdLockD3D"
	,L"NtGdiDdUnlockD3D"
	,L"NtGdiGetCharWidthW"
	,L"NtUserInvalidateRgn"
	,L"NtUserGetClipboardOwner"
	,L"NtUserSetWindowRgn"
	,L"NtUserBitBltSysBmp"
	,L"NtGdiGetCharWidthInfo"
	,L"NtUserValidateRect"
	,L"NtUserCloseClipboard"
	,L"NtUserOpenClipboard"
	,L"NtGdiGetStockObject"
	,L"NtUserSetClipboardData"
	,L"NtUserEnableMenuItem"
	,L"NtUserAlterWindowStyle"
	,L"NtGdiFillRgn"
	,L"NtUserGetWindowPlacement"
	,L"NtGdiModifyWorldTransform"
	,L"NtGdiGetFontData"
	,L"NtUserGetOpenClipboardWindow"
	,L"NtUserSetThreadState"
	,L"NtGdiOpenDCW"
	,L"NtUserTrackMouseEvent"
	,L"NtGdiGetTransform"
	,L"NtUserDestroyMenu"
	,L"NtGdiGetBitmapBits"
	,L"NtUserConsoleControl"
	,L"NtUserSetActiveWindow"
	,L"NtUserSetInformationThread"
	,L"NtUserSetWindowPlacement"
	,L"NtUserGetControlColor"
	,L"NtGdiSetMetaRgn"
	,L"NtGdiSetMiterLimit"
	,L"NtGdiSetVirtualResolution"
	,L"NtGdiGetRasterizerCaps"
	,L"NtUserSetWindowWord"
	,L"NtUserGetClipboardFormatName"
	,L"NtUserRealInternalGetMessage"
	,L"NtUserCreateLocalMemHandle"
	,L"NtUserAttachThreadInput"
	,L"NtGdiCreateHalftonePalette"
	,L"NtUserPaintMenuBar"
	,L"NtUserSetKeyboardState"
	,L"NtGdiCombineTransform"
	,L"NtUserCreateAcceleratorTable"
	,L"NtUserGetCursorFrameInfo"
	,L"NtUserGetAltTabInfo"
	,L"NtUserGetCaretBlinkTime"
	,L"NtGdiQueryFontAssocInfo"
	,L"NtUserProcessConnect"
	,L"NtUserEnumDisplayDevices"
	,L"NtUserEmptyClipboard"
	,L"NtUserGetClipboardData"
	,L"NtUserRemoveMenu"
	,L"NtGdiSetBoundsRect"
	,L"NtGdiGetBitmapDimension"
	,L"NtUserConvertMemHandle"
	,L"NtUserDestroyAcceleratorTable"
	,L"NtUserGetGUIThreadInfo"
	,L"NtGdiCloseFigure"
	,L"NtUserSetWindowsHookAW"
	,L"NtUserSetMenuDefaultItem"
	,L"NtUserCheckMenuItem"
	,L"NtUserSetWinEventHook"
	,L"NtUserUnhookWinEvent"
	,L"NtUserLockWindowUpdate"
	,L"NtUserSetSystemMenu"
	,L"NtUserThunkedMenuInfo"
	,L"NtGdiBeginPath"
	,L"NtGdiEndPath"
	,L"NtGdiFillPath"
	,L"NtUserCallHwnd"
	,L"NtUserDdeInitialize"
	,L"NtUserModifyUserStartupInfoFlags"
	,L"NtUserCountClipboardFormats"
	,L"NtGdiAddFontMemResourceEx"
	,L"NtGdiEqualRgn"
	,L"NtGdiGetSystemPaletteUse"
	,L"NtGdiRemoveFontMemResourceEx"
	,L"NtUserEnumDisplaySettings"
	,L"NtUserPaintDesktop"
	,L"NtGdiExtEscape"
	,L"NtGdiSetBitmapDimension"
	,L"NtGdiSetFontEnumeration"
	,L"NtUserChangeClipboardChain"
	,L"NtUserSetClipboardViewer"
	,L"NtUserShowWindowAsync"
	,L"NtGdiCreateColorSpace"
	,L"NtGdiDeleteColorSpace"
	,L"NtUserActivateKeyboardLayout"
	,L"NtGdiAbortDoc"
	,L"NtGdiAbortPath"
	,L"NtGdiAddEmbFontToDC"
	,L"NtGdiAddFontResourceW"
	,L"NtGdiAddRemoteFontToDC"
	,L"NtGdiAddRemoteMMInstanceToDC"
	,L"NtGdiAngleArc"
	,L"NtGdiAnyLinkedFonts"
	,L"NtGdiArcInternal"
	,L"NtGdiBRUSHOBJ_DeleteRbrush"
	,L"NtGdiBRUSHOBJ_hGetColorTransform"
	,L"NtGdiBRUSHOBJ_pvAllocRbrush"
	,L"NtGdiBRUSHOBJ_pvGetRbrush"
	,L"NtGdiBRUSHOBJ_ulGetBrushColor"
	,L"NtGdiBeginGdiRendering"
	,L"NtGdiCLIPOBJ_bEnum"
	,L"NtGdiCLIPOBJ_cEnumStart"
	,L"NtGdiCLIPOBJ_ppoGetPath"
	,L"NtGdiCancelDC"
	,L"NtGdiChangeGhostFont"
	,L"NtGdiCheckBitmapBits"
	,L"NtGdiClearBitmapAttributes"
	,L"NtGdiClearBrushAttributes"
	,L"NtGdiColorCorrectPalette"
	,L"NtGdiConfigureOPMProtectedOutput"
	,L"NtGdiConvertMetafileRect"
	,L"NtGdiCreateBitmapFromDxSurface"
	,L"NtGdiCreateColorTransform"
	,L"NtGdiCreateEllipticRgn"
	,L"NtGdiCreateHatchBrushInternal"
	,L"NtGdiCreateMetafileDC"
	,L"NtGdiCreateOPMProtectedOutputs"
	,L"NtGdiCreateRoundRectRgn"
	,L"NtGdiCreateServerMetaFile"
	,L"NtGdiD3dContextCreate"
	,L"NtGdiD3dContextDestroy"
	,L"NtGdiD3dContextDestroyAll"
	,L"NtGdiD3dValidateTextureStageState"
	,L"NtGdiDDCCIGetCapabilitiesString"
	,L"NtGdiDDCCIGetCapabilitiesStringLength"
	,L"NtGdiDDCCIGetTimingReport"
	,L"NtGdiDDCCIGetVCPFeature"
	,L"NtGdiDDCCISaveCurrentSettings"
	,L"NtGdiDDCCISetVCPFeature"
	,L"NtGdiDdAddAttachedSurface"
	,L"NtGdiDdAlphaBlt"
	,L"NtGdiDdAttachSurface"
	,L"NtGdiDdBeginMoCompFrame"
	,L"NtGdiDdCanCreateD3DBuffer"
	,L"NtGdiDdColorControl"
	,L"NtGdiDdCreateD3DBuffer"
	,L"NtGdiDdCreateDirectDrawObject"
	,L"NtGdiDdCreateFullscreenSprite"
	,L"NtGdiDdCreateMoComp"
	,L"NtGdiDdDDIAcquireKeyedMutex"
	,L"NtGdiDdDDICheckExclusiveOwnership"
	,L"NtGdiDdDDICheckMonitorPowerState"
	,L"NtGdiDdDDICheckOcclusion"
	,L"NtGdiDdDDICheckSharedResourceAccess"
	,L"NtGdiDdDDICheckVidPnExclusiveOwnership"
	,L"NtGdiDdDDICloseAdapter"
	,L"NtGdiDdDDIConfigureSharedResource"
	,L"NtGdiDdDDICreateAllocation"
	,L"NtGdiDdDDICreateContext"
	,L"NtGdiDdDDICreateDCFromMemory"
	,L"NtGdiDdDDICreateDevice"
	,L"NtGdiDdDDICreateKeyedMutex"
	,L"NtGdiDdDDICreateOverlay"
	,L"NtGdiDdDDICreateSynchronizationObject"
	,L"NtGdiDdDDIDestroyAllocation"
	,L"NtGdiDdDDIDestroyContext"
	,L"NtGdiDdDDIDestroyDCFromMemory"
	,L"NtGdiDdDDIDestroyDevice"
	,L"NtGdiDdDDIDestroyKeyedMutex"
	,L"NtGdiDdDDIDestroyOverlay"
	,L"NtGdiDdDDIDestroySynchronizationObject"
	,L"NtGdiDdDDIEscape"
	,L"NtGdiDdDDIFlipOverlay"
	,L"NtGdiDdDDIGetContextSchedulingPriority"
	,L"NtGdiDdDDIGetDeviceState"
	,L"NtGdiDdDDIGetDisplayModeList"
	,L"NtGdiDdDDIGetMultisampleMethodList"
	,L"NtGdiDdDDIGetOverlayState"
	,L"NtGdiDdDDIGetPresentHistory"
	,L"NtGdiDdDDIGetPresentQueueEvent"
	,L"NtGdiDdDDIGetProcessSchedulingPriorityClass"
	,L"NtGdiDdDDIGetRuntimeData"
	,L"NtGdiDdDDIGetScanLine"
	,L"NtGdiDdDDIGetSharedPrimaryHandle"
	,L"NtGdiDdDDIInvalidateActiveVidPn"
	,L"NtGdiDdDDILock"
	,L"NtGdiDdDDIOpenAdapterFromDeviceName"
	,L"NtGdiDdDDIOpenAdapterFromHdc"
	,L"NtGdiDdDDIOpenKeyedMutex"
	,L"NtGdiDdDDIOpenResource"
	,L"NtGdiDdDDIOpenSynchronizationObject"
	,L"NtGdiDdDDIPollDisplayChildren"
	,L"NtGdiDdDDIPresent"
	,L"NtGdiDdDDIQueryAdapterInfo"
	,L"NtGdiDdDDIQueryAllocationResidency"
	,L"NtGdiDdDDIQueryResourceInfo"
	,L"NtGdiDdDDIQueryStatistics"
	,L"NtGdiDdDDIReleaseKeyedMutex"
	,L"NtGdiDdDDIReleaseProcessVidPnSourceOwners"
	,L"NtGdiDdDDIRender"
	,L"NtGdiDdDDISetAllocationPriority"
	,L"NtGdiDdDDISetContextSchedulingPriority"
	,L"NtGdiDdDDISetDisplayMode"
	,L"NtGdiDdDDISetDisplayPrivateDriverFormat"
	,L"NtGdiDdDDISetGammaRamp"
	,L"NtGdiDdDDISetProcessSchedulingPriorityClass"
	,L"NtGdiDdDDISetQueuedLimit"
	,L"NtGdiDdDDISetVidPnSourceOwner"
	,L"NtGdiDdDDISharedPrimaryLockNotification"
	,L"NtGdiDdDDISharedPrimaryUnLockNotification"
	,L"NtGdiDdDDISignalSynchronizationObject"
	,L"NtGdiDdDDIUnlock"
	,L"NtGdiDdDDIUpdateOverlay"
	,L"NtGdiDdDDIWaitForIdle"
	,L"NtGdiDdDDIWaitForSynchronizationObject"
	,L"NtGdiDdDDIWaitForVerticalBlankEvent"
	,L"NtGdiDdDeleteDirectDrawObject"
	,L"NtGdiDdDestroyD3DBuffer"
	,L"NtGdiDdDestroyFullscreenSprite"
	,L"NtGdiDdDestroyMoComp"
	,L"NtGdiDdEndMoCompFrame"
	,L"NtGdiDdFlip"
	,L"NtGdiDdFlipToGDISurface"
	,L"NtGdiDdGetAvailDriverMemory"
	,L"NtGdiDdGetBltStatus"
	,L"NtGdiDdGetDC"
	,L"NtGdiDdGetDriverInfo"
	,L"NtGdiDdGetDriverState"
	,L"NtGdiDdGetDxHandle"
	,L"NtGdiDdGetFlipStatus"
	,L"NtGdiDdGetInternalMoCompInfo"
	,L"NtGdiDdGetMoCompBuffInfo"
	,L"NtGdiDdGetMoCompFormats"
	,L"NtGdiDdGetMoCompGuids"
	,L"NtGdiDdGetScanLine"
	,L"NtGdiDdLock"
	,L"NtGdiDdNotifyFullscreenSpriteUpdate"
	,L"NtGdiDdQueryDirectDrawObject"
	,L"NtGdiDdQueryMoCompStatus"
	,L"NtGdiDdQueryVisRgnUniqueness"
	,L"NtGdiDdReenableDirectDrawObject"
	,L"NtGdiDdReleaseDC"
	,L"NtGdiDdRenderMoComp"
	,L"NtGdiDdSetColorKey"
	,L"NtGdiDdSetExclusiveMode"
	,L"NtGdiDdSetGammaRamp"
	,L"NtGdiDdSetOverlayPosition"
	,L"NtGdiDdUnattachSurface"
	,L"NtGdiDdUnlock"
	,L"NtGdiDdUpdateOverlay"
	,L"NtGdiDdWaitForVerticalBlank"
	,L"NtGdiDeleteColorTransform"
	,L"NtGdiDescribePixelFormat"
	,L"NtGdiDestroyOPMProtectedOutput"
	,L"NtGdiDestroyPhysicalMonitor"
	,L"NtGdiDoBanding"
	,L"NtGdiDrawEscape"
	,L"NtGdiDvpAcquireNotification"
	,L"NtGdiDvpCanCreateVideoPort"
	,L"NtGdiDvpColorControl"
	,L"NtGdiDvpCreateVideoPort"
	,L"NtGdiDvpDestroyVideoPort"
	,L"NtGdiDvpFlipVideoPort"
	,L"NtGdiDvpGetVideoPortBandwidth"
	,L"NtGdiDvpGetVideoPortConnectInfo"
	,L"NtGdiDvpGetVideoPortField"
	,L"NtGdiDvpGetVideoPortFlipStatus"
	,L"NtGdiDvpGetVideoPortInputFormats"
	,L"NtGdiDvpGetVideoPortLine"
	,L"NtGdiDvpGetVideoPortOutputFormats"
	,L"NtGdiDvpGetVideoSignalStatus"
	,L"NtGdiDvpReleaseNotification"
	,L"NtGdiDvpUpdateVideoPort"
	,L"NtGdiDvpWaitForVideoPortSync"
	,L"NtGdiDxgGenericThunk"
	,L"NtGdiEllipse"
	,L"NtGdiEnableEudc"
	,L"NtGdiEndDoc"
	,L"NtGdiEndGdiRendering"
	,L"NtGdiEndPage"
	,L"NtGdiEngAlphaBlend"
	,L"NtGdiEngAssociateSurface"
	,L"NtGdiEngBitBlt"
	,L"NtGdiEngCheckAbort"
	,L"NtGdiEngComputeGlyphSet"
	,L"NtGdiEngCopyBits"
	,L"NtGdiEngCreateBitmap"
	,L"NtGdiEngCreateClip"
	,L"NtGdiEngCreateDeviceBitmap"
	,L"NtGdiEngCreateDeviceSurface"
	,L"NtGdiEngCreatePalette"
	,L"NtGdiEngDeleteClip"
	,L"NtGdiEngDeletePalette"
	,L"NtGdiEngDeletePath"
	,L"NtGdiEngDeleteSurface"
	,L"NtGdiEngEraseSurface"
	,L"NtGdiEngFillPath"
	,L"NtGdiEngGradientFill"
	,L"NtGdiEngLineTo"
	,L"NtGdiEngLockSurface"
	,L"NtGdiEngMarkBandingSurface"
	,L"NtGdiEngPaint"
	,L"NtGdiEngPlgBlt"
	,L"NtGdiEngStretchBlt"
	,L"NtGdiEngStretchBltROP"
	,L"NtGdiEngStrokeAndFillPath"
	,L"NtGdiEngStrokePath"
	,L"NtGdiEngTextOut"
	,L"NtGdiEngTransparentBlt"
	,L"NtGdiEngUnlockSurface"
	,L"NtGdiEnumFonts"
	,L"NtGdiEnumObjects"
	,L"NtGdiEudcLoadUnloadLink"
	,L"NtGdiExtFloodFill"
	,L"NtGdiFONTOBJ_cGetAllGlyphHandles"
	,L"NtGdiFONTOBJ_cGetGlyphs"
	,L"NtGdiFONTOBJ_pQueryGlyphAttrs"
	,L"NtGdiFONTOBJ_pfdg"
	,L"NtGdiFONTOBJ_pifi"
	,L"NtGdiFONTOBJ_pvTrueTypeFontFile"
	,L"NtGdiFONTOBJ_pxoGetXform"
	,L"NtGdiFONTOBJ_vGetInfo"
	,L"NtGdiFlattenPath"
	,L"NtGdiFontIsLinked"
	,L"NtGdiForceUFIMapping"
	,L"NtGdiFrameRgn"
	,L"NtGdiFullscreenControl"
	,L"NtGdiGetBoundsRect"
	,L"NtGdiGetCOPPCompatibleOPMInformation"
	,L"NtGdiGetCertificate"
	,L"NtGdiGetCertificateSize"
	,L"NtGdiGetCharABCWidthsW"
	,L"NtGdiGetCharacterPlacementW"
	,L"NtGdiGetColorAdjustment"
	,L"NtGdiGetColorSpaceforBitmap"
	,L"NtGdiGetDeviceCaps"
	,L"NtGdiGetDeviceCapsAll"
	,L"NtGdiGetDeviceGammaRamp"
	,L"NtGdiGetDeviceWidth"
	,L"NtGdiGetDhpdev"
	,L"NtGdiGetETM"
	,L"NtGdiGetEmbUFI"
	,L"NtGdiGetEmbedFonts"
	,L"NtGdiGetEudcTimeStampEx"
	,L"NtGdiGetFontFileData"
	,L"NtGdiGetFontFileInfo"
	,L"NtGdiGetFontResourceInfoInternalW"
	,L"NtGdiGetFontUnicodeRanges"
	,L"NtGdiGetGlyphIndicesW"
	,L"NtGdiGetGlyphIndicesWInternal"
	,L"NtGdiGetGlyphOutline"
	,L"NtGdiGetKerningPairs"
	,L"NtGdiGetLinkedUFIs"
	,L"NtGdiGetMiterLimit"
	,L"NtGdiGetMonitorID"
	,L"NtGdiGetNumberOfPhysicalMonitors"
	,L"NtGdiGetOPMInformation"
	,L"NtGdiGetOPMRandomNumber"
	,L"NtGdiGetObjectBitmapHandle"
	,L"NtGdiGetPath"
	,L"NtGdiGetPerBandInfo"
	,L"NtGdiGetPhysicalMonitorDescription"
	,L"NtGdiGetPhysicalMonitors"
	,L"NtGdiGetRealizationInfo"
	,L"NtGdiGetServerMetaFileBits"
	,L"DxgStubAlphaBlt"
	,L"NtGdiGetStats"
	,L"NtGdiGetStringBitmapW"
	,L"NtGdiGetSuggestedOPMProtectedOutputArraySize"
	,L"NtGdiGetTextExtentExW"
	,L"NtGdiGetUFI"
	,L"NtGdiGetUFIPathname"
	,L"NtGdiGradientFill"
	,L"NtGdiHLSurfGetInformation"
	,L"NtGdiHLSurfSetInformation"
	,L"NtGdiHT_Get8BPPFormatPalette"
	,L"NtGdiHT_Get8BPPMaskPalette"
	,L"NtGdiIcmBrushInfo"
	,L"EngRestoreFloatingPointState"
	,L"NtGdiInitSpool"
	,L"NtGdiMakeFontDir"
	,L"NtGdiMakeInfoDC"
	,L"NtGdiMakeObjectUnXferable"
	,L"NtGdiMakeObjectXferable"
	,L"NtGdiMirrorWindowOrg"
	,L"NtGdiMonoBitmap"
	,L"NtGdiMoveTo"
	,L"NtGdiOffsetClipRgn"
	,L"NtGdiPATHOBJ_bEnum"
	,L"NtGdiPATHOBJ_bEnumClipLines"
	,L"NtGdiPATHOBJ_vEnumStart"
	,L"NtGdiPATHOBJ_vEnumStartClipLines"
	,L"NtGdiPATHOBJ_vGetBounds"
	,L"NtGdiPathToRegion"
	,L"NtGdiPlgBlt"
	,L"NtGdiPolyDraw"
	,L"NtGdiPolyTextOutW"
	,L"NtGdiPtInRegion"
	,L"NtGdiPtVisible"
	,L"NtGdiQueryFonts"
	,L"NtGdiRemoveFontResourceW"
	,L"NtGdiRemoveMergeFont"
	,L"NtGdiResetDC"
	,L"NtGdiResizePalette"
	,L"NtGdiRoundRect"
	,L"NtGdiSTROBJ_bEnum"
	,L"NtGdiSTROBJ_bEnumPositionsOnly"
	,L"NtGdiSTROBJ_bGetAdvanceWidths"
	,L"NtGdiSTROBJ_dwGetCodePage"
	,L"NtGdiSTROBJ_vEnumStart"
	,L"NtGdiScaleViewportExtEx"
	,L"NtGdiScaleWindowExtEx"
	,L"NtGdiSelectBrush"
	,L"NtGdiSelectClipPath"
	,L"NtGdiSelectPen"
	,L"NtGdiSetBitmapAttributes"
	,L"NtGdiSetBrushAttributes"
	,L"NtGdiSetColorAdjustment"
	,L"NtGdiSetColorSpace"
	,L"NtGdiSetDeviceGammaRamp"
	,L"NtGdiSetFontXform"
	,L"NtGdiSetIcmMode"
	,L"NtGdiSetLinkedUFIs"
	,L"NtGdiSetMagicColors"
	,L"NtGdiSetOPMSigningKeyAndSequenceNumbers"
	,L"NtGdiSetPUMPDOBJ"
	,L"NtGdiSetPixelFormat"
	,L"NtGdiSetRectRgn"
	,L"NtGdiSetSizeDevice"
	,L"NtGdiSetSystemPaletteUse"
	,L"NtGdiSetTextJustification"
	,L"NtGdiSfmGetNotificationTokens"
	,L"NtGdiStartDoc"
	,L"NtGdiStartPage"
	,L"NtGdiStrokeAndFillPath"
	,L"NtGdiStrokePath"
	,L"NtGdiSwapBuffers"
	,L"NtGdiTransparentBlt"
	,L"NtGdiUMPDEngFreeUserMem"
	,L"DxgStubAlphaBlt"
	,L"EngRestoreFloatingPointState"
	,L"NtGdiUpdateColors"
	,L"NtGdiUpdateTransform"
	,L"NtGdiWidenPath"
	,L"NtGdiXFORMOBJ_bApplyXform"
	,L"NtGdiXFORMOBJ_iGetXform"
	,L"NtGdiXLATEOBJ_cGetPalette"
	,L"NtGdiXLATEOBJ_hGetColorTransform"
	,L"NtGdiXLATEOBJ_iXlate"
	,L"NtUserAddClipboardFormatListener"
	,L"NtUserAssociateInputContext"
	,L"NtUserBlockInput"
	,L"NtUserBuildHimcList"
	,L"NtUserBuildPropList"
	,L"NtUserCalculatePopupWindowPosition"
	,L"NtUserCallHwndOpt"
	,L"NtUserChangeDisplaySettings"
	,L"NtUserChangeWindowMessageFilterEx"
	,L"NtUserCheckAccessForIntegrityLevel"
	,L"NtUserCheckDesktopByThreadId"
	,L"NtUserCheckWindowThreadDesktop"
	,L"NtUserChildWindowFromPointEx"
	,L"NtUserClipCursor"
	,L"NtUserCreateDesktopEx"
	,L"NtUserCreateInputContext"
	,L"NtUserCreateWindowStation"
	,L"NtUserCtxDisplayIOCtl"
	,L"NtUserDestroyInputContext"
	,L"NtUserDisableThreadIme"
	,L"NtUserDisplayConfigGetDeviceInfo"
	,L"NtUserDisplayConfigSetDeviceInfo"
	,L"NtUserDoSoundConnect"
	,L"NtUserDoSoundDisconnect"
	,L"NtUserDragDetect"
	,L"NtUserDragObject"
	,L"NtUserDrawAnimatedRects"
	,L"NtUserDrawCaption"
	,L"NtUserDrawCaptionTemp"
	,L"NtUserDrawMenuBarTemp"
	,L"NtUserDwmStartRedirection"
	,L"NtUserDwmStopRedirection"
	,L"NtUserEndMenu"
	,L"NtUserEndTouchOperation"
	,L"NtUserEvent"
	,L"NtUserFlashWindowEx"
	,L"NtUserFrostCrashedWindow"
	,L"NtUserGetAppImeLevel"
	,L"NtUserGetCaretPos"
	,L"NtUserGetClipCursor"
	,L"NtUserGetClipboardViewer"
	,L"NtUserGetComboBoxInfo"
	,L"NtUserGetCursorInfo"
	,L"NtUserGetDisplayConfigBufferSizes"
	,L"NtUserGetGestureConfig"
	,L"NtUserGetGestureExtArgs"
	,L"NtUserGetGestureInfo"
	,L"NtUserGetGuiResources"
	,L"NtUserGetImeHotKey"
	,L"NtUserGetImeInfoEx"
	,L"NtUserGetInputLocaleInfo"
	,L"NtUserGetInternalWindowPos"
	,L"NtUserGetKeyNameText"
	,L"NtUserGetKeyboardLayoutName"
	,L"NtUserGetLayeredWindowAttributes"
	,L"NtUserGetListBoxInfo"
	,L"NtUserGetMenuIndex"
	,L"NtUserGetMenuItemRect"
	,L"NtUserGetMouseMovePointsEx"
	,L"NtUserGetPriorityClipboardFormat"
	,L"NtUserGetRawInputBuffer"
	,L"NtUserGetRawInputData"
	,L"NtUserGetRawInputDeviceInfo"
	,L"NtUserGetRawInputDeviceList"
	,L"NtUserGetRegisteredRawInputDevices"
	,L"NtUserGetTopLevelWindow"
	,L"NtUserGetTouchInputInfo"
	,L"NtUserGetUpdatedClipboardFormats"
	,L"NtUserGetWOWClass"
	,L"NtUserGetWindowCompositionAttribute"
	,L"NtUserGetWindowCompositionInfo"
	,L"NtUserGetWindowDisplayAffinity"
	,L"NtUserGetWindowMinimizeRect"
	,L"NtUserGetWindowRgnEx"
	,L"NtUserGhostWindowFromHungWindow"
	,L"NtUserHardErrorControl"
	,L"NtUserHiliteMenuItem"
	,L"NtUserHungWindowFromGhostWindow"
	,L"NtUserHwndQueryRedirectionInfo"
	,L"NtUserHwndSetRedirectionInfo"
	,L"NtUserImpersonateDdeClientWindow"
	,L"NtUserInitTask"
	,L"NtUserInitialize"
	,L"NtUserInitializeClientPfnArrays"
	,L"NtUserInjectGesture"
	,L"NtUserInternalGetWindowIcon"
	,L"NtUserIsTopLevelWindow"
	,L"NtUserIsTouchWindow"
	,L"NtUserLoadKeyboardLayoutEx"
	,L"NtUserLockWindowStation"
	,L"NtUserLockWorkStation"
	,L"NtUserLogicalToPhysicalPoint"
	,L"NtUserMNDragLeave"
	,L"NtUserMNDragOver"
	,L"NtUserMagControl"
	,L"NtUserMagGetContextInformation"
	,L"NtUserMagSetContextInformation"
	,L"NtUserManageGestureHandlerWindow"
	,L"NtUserMenuItemFromPoint"
	,L"NtUserMinMaximize"
	,L"NtUserModifyWindowTouchCapability"
	,L"NtUserNotifyIMEStatus"
	,L"NtUserOpenInputDesktop"
	,L"NtUserOpenThreadDesktop"
	,L"NtUserPaintMonitor"
	,L"NtUserPhysicalToLogicalPoint"
	,L"NtUserPrintWindow"
	,L"NtUserQueryDisplayConfig"
	,L"NtUserQueryInformationThread"
	,L"NtUserQueryInputContext"
	,L"NtUserQuerySendMessage"
	,L"NtUserRealChildWindowFromPoint"
	,L"NtUserRealWaitMessageEx"
	,L"NtUserRegisterErrorReportingDialog"
	,L"NtUserRegisterHotKey"
	,L"NtUserRegisterRawInputDevices"
	,L"NtUserRegisterServicesProcess"
	,L"NtUserRegisterSessionPort"
	,L"NtUserRegisterTasklist"
	,L"NtUserRegisterUserApiHook"
	,L"NtUserRemoteConnect"
	,L"NtUserRemoteRedrawRectangle"
	,L"NtUserRemoteRedrawScreen"
	,L"NtUserRemoteStopScreenUpdates"
	,L"NtUserRemoveClipboardFormatListener"
	,L"NtUserResolveDesktopForWOW"
	,L"NtUserSendTouchInput"
	,L"NtUserSetAppImeLevel"
	,L"NtUserSetChildWindowNoActivate"
	,L"NtUserSetClassWord"
	,L"NtUserSetCursorContents"
	,L"NtUserSetDisplayConfig"
	,L"NtUserSetGestureConfig"
	,L"NtUserSetImeHotKey"
	,L"NtUserSetImeInfoEx"
	,L"NtUserSetImeOwnerWindow"
	,L"NtUserSetInternalWindowPos"
	,L"NtUserSetLayeredWindowAttributes"
	,L"NtUserSetMenu"
	,L"NtUserSetMenuContextHelpId"
	,L"NtUserSetMenuFlagRtoL"
	,L"NtUserSetMirrorRendering"
	,L"NtUserSetObjectInformation"
	,L"NtUserSetProcessDPIAware"
	,L"NtUserSetShellWindowEx"
	,L"NtUserSetSysColors"
	,L"NtUserSetSystemCursor"
	,L"NtUserSetSystemTimer"
	,L"NtUserSetThreadLayoutHandles"
	,L"NtUserSetWindowCompositionAttribute"
	,L"NtUserSetWindowDisplayAffinity"
	,L"NtUserSetWindowRgnEx"
	,L"NtUserSetWindowStationUser"
	,L"NtUserSfmDestroyLogicalSurfaceBinding"
	,L"NtUserSfmDxBindSwapChain"
	,L"NtUserSfmDxGetSwapChainStats"
	,L"NtUserSfmDxOpenSwapChain"
	,L"NtUserSfmDxQuerySwapChainBindingStatus"
	,L"NtUserSfmDxReleaseSwapChain"
	,L"NtUserSfmDxReportPendingBindingsToDwm"
	,L"NtUserSfmDxSetSwapChainBindingStatus"
	,L"NtUserSfmDxSetSwapChainStats"
	,L"NtUserSfmGetLogicalSurfaceBinding"
	,L"NtUserShowSystemCursor"
	,L"NtUserSoundSentry"
	,L"NtUserSwitchDesktop"
	,L"NtUserTestForInteractiveUser"
	,L"NtUserTrackPopupMenuEx"
	,L"NtUserUnloadKeyboardLayout"
	,L"NtUserUnlockWindowStation"
	,L"NtUserUnregisterHotKey"
	,L"NtUserUnregisterSessionPort"
	,L"NtUserUnregisterUserApiHook"
	,L"NtUserUpdateInputContext"
	,L"NtUserUpdateInstance"
	,L"NtUserUpdateLayeredWindow"
	,L"NtUserUpdatePerUserSystemParameters"
	,L"NtUserUpdateWindowTransform"
	,L"NtUserUserHandleGrantAccess"
	,L"NtUserValidateHandleSecure"
	,L"NtUserWaitForInputIdle"
	,L"NtUserWaitForMsgAndEvent"
	,L"NtUserWindowFromPhysicalPoint"
	,L"NtUserYieldTask"
	,L"NtUserSetClassLongPtr"
	,L"NtUserSetWindowLongPtr"
};

/************************************************************************
*  Name : APGetCurrentSssdtAddress
*  Param: void
*  Ret  : UINT_PTR
*  获得SSSDT地址 （x86 搜索导出表/x64 硬编码，算偏移）
************************************************************************/
UINT_PTR
APGetCurrentSssdtAddress()
{
	UINT_PTR CurrentSssdtAddress = 0;

#ifdef _WIN64
	/*
	kd> rdmsr c0000082
	msr[c0000082] = fffff800`03e81640
	*/
	PUINT8	StartSearchAddress = (PUINT8)__readmsr(0xC0000082);   // fffff800`03ecf640
	PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
	PUINT8	i = NULL;
	UINT8   v1 = 0, v2 = 0, v3 = 0;
	INT32   iOffset = 0;    // 002320c7 偏移不会超过4字节

	for (i = StartSearchAddress; i<EndSearchAddress; i++)
	{
		/*
		kd> u fffff800`03e81640 l 500
		nt!KiSystemCall64:
		fffff800`03e81640 0f01f8          swapgs
		......

		nt!KiSystemServiceRepeat:
		fffff800`03e9c772 4c8d15c7202300  lea     r10,[nt!KeServiceDescriptorTable (fffff800`040ce840)]
		fffff800`03e9c779 4c8d1d00212300  lea     r11,[nt!KeServiceDescriptorTableShadow (fffff800`040ce880)]
		fffff800`03e9c780 f7830001000080000000 test dword ptr [rbx+100h],80h

		TargetAddress = CurrentAddress + Offset + 7
		fffff800`040ce840 = fffff800`03e9c772 + 0x002320c7 + 7
		*/

		if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
		{
			v1 = *i;
			v2 = *(i + 1);
			v3 = *(i + 2);
			if (v1 == 0x4c && v2 == 0x8d && v3 == 0x1d)		// 硬编码  lea r11
			{
				RtlCopyMemory(&iOffset, i + 3, 4);
				CurrentSssdtAddress = (UINT_PTR)(iOffset + (UINT64)i + 7);
				break;
			}
		}
	}

#else
	UINT32 KeAddSystemServiceTableAddress = NULL;

	APGetNtosExportVariableAddress(L"KeAddSystemServiceTable", (PVOID*)&KeAddSystemServiceTableAddress);
	if (KeAddSystemServiceTableAddress)
	{
		PUINT8	StartSearchAddress = (PUINT8)KeAddSystemServiceTableAddress;
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 002320c7 偏移不会超过4字节

		for (i = StartSearchAddress; i<EndSearchAddress; i++)
		{
			/*
			kd> u fffff800`03e81640 l 500
			nt!KiSystemCall64:
			fffff800`03e81640 0f01f8          swapgs
			......

			nt!KiSystemServiceRepeat:
			fffff800`03e9c772 4c8d15c7202300  lea     r10,[nt!KeServiceDescriptorTable (fffff800`040ce840)]
			fffff800`03e9c779 4c8d1d00212300  lea     r11,[nt!KeServiceDescriptorTableShadow (fffff800`040ce880)]
			fffff800`03e9c780 f7830001000080000000 test dword ptr [rbx+100h],80h

			TargetAddress = CurrentAddress + Offset + 7
			fffff800`040ce840 = fffff800`03e9c772 + 0x002320c7 + 7
			*/

			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x4c && v2 == 0x8d && v3 == 0x1d)		// 硬编码  lea r11
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					(UINT_PTR)g_CurrentSssdtAddress = (UINT_PTR)(iOffset + (UINT64)i + 7);

					(UINT_PTR)g_CurrentSssdtAddress += sizeof(KSERVICE_TABLE_DESCRIPTOR);      // 过Ssdt
				}
			}
		}

	}

#endif

	DbgPrint("SSDTAddress is %p\r\n", CurrentSssdtAddress);

	return CurrentSssdtAddress;
}


UINT_PTR
APGetCurrentWin32pServiceTable()
{
	/*
	kd> dq fffff800`040be980
	fffff800`040be980  fffff800`03e87800 00000000`00000000
	fffff800`040be990  00000000`00000191 fffff800`03e8848c
	fffff800`040be9a0  fffff960`000e1f00 00000000`00000000
	fffff800`040be9b0  00000000`0000033b fffff960`000e3c1c

	kd> dq win32k!W32pServiceTable
	fffff960`000e1f00  fff0b501`fff3a740 001021c0`000206c0
	fffff960`000e1f10  00022640`00096000 ffde0b03`fff9a900

	*/

	if (g_CurrentWin32pServiceTableAddress == NULL)
	{
		(UINT_PTR)g_CurrentWin32pServiceTableAddress = APGetCurrentSssdtAddress() + sizeof(KSERVICE_TABLE_DESCRIPTOR);    // 过Ssdt 
	}

	return (UINT_PTR)g_CurrentWin32pServiceTableAddress;
}


/************************************************************************
*  Name : APFixWin32pServiceTable
*  Param: ImageBase			    新模块加载基地址 （PVOID）
*  Param: OriginalBase		    原模块加载基地址 （PVOID）
*  Ret  : VOID
*  修正Win32pServiceTable 以及base里面的函数
************************************************************************/
VOID
APFixWin32pServiceTable(IN PVOID ImageBase, IN PVOID OriginalBase)
{
	UINT_PTR KrnlOffset = (INT64)((UINT_PTR)ImageBase - (UINT_PTR)OriginalBase);

	DbgPrint("Krnl Offset :%x\r\n", KrnlOffset);

	// 给SSDT赋值

	g_ReloadWin32pServiceTableAddress.Base = (PUINT_PTR)((UINT_PTR)(g_CurrentWin32pServiceTableAddress->Base) + KrnlOffset);
	g_ReloadWin32pServiceTableAddress.Limit = g_CurrentWin32pServiceTableAddress->Limit;
	g_ReloadWin32pServiceTableAddress.Number = g_CurrentWin32pServiceTableAddress->Number;

	DbgPrint("New Win32pServiceTable:%p\r\n", g_ReloadWin32pServiceTableAddress);
	DbgPrint("New Win32pServiceTable Base:%p\r\n", g_ReloadWin32pServiceTableAddress.Base);

	// 给Base里的每个成员赋值（函数地址）
	if (MmIsAddressValid(g_ReloadWin32pServiceTableAddress.Base))
	{

#ifdef _WIN64

		// 刚开始保存的是函数的真实地址，我们保存在自己的全局数组中
		for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			g_OriginalSssdtFunctionAddress[i] = *(UINT64*)((UINT_PTR)g_ReloadWin32pServiceTableAddress.Base + i * 8);
		}

		for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			UINT32 Temp = 0;
			Temp = (UINT32)(g_OriginalSssdtFunctionAddress[i] - (UINT64)g_CurrentWin32pServiceTableAddress->Base);
			Temp += ((UINT64)g_CurrentWin32pServiceTableAddress->Base & 0xffffffff);
			// 更新Ssdt->base中的成员为相对于Base的偏移
			*(UINT32*)((UINT64)g_ReloadWin32pServiceTableAddress.Base + i * 4) = (Temp - ((UINT64)g_CurrentWin32pServiceTableAddress->Base & 0xffffffff)) << 4;
		}

		DbgPrint("Current%p\n", g_CurrentWin32pServiceTableAddress->Base);
		DbgPrint("Reload%p\n", g_ReloadWin32pServiceTableAddress.Base);

	/*	for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			g_SssdtItem[i] = *(UINT32*)((UINT64)g_ReloadWin32pServiceTableAddress.Base + i * 4);
		}*/
#else
		for (UINT32 i = 0; i < g_ReloadWin32pServiceTableAddress.Limit; i++)
		{
			g_OriginalSssdtFunctionAddress[i] = *(UINT32*)(g_ReloadWin32pServiceTableAddress.Base + i * 4);
			//g_SssdtItem[i] = g_OriginalSssdtFunctionAddress[i];
			*(UINT32*)(g_ReloadWin32pServiceTableAddress.Base + i * 4) += KrnlOffset;      // 将所有Ssdt函数地址转到我们新加载到内存中的地址
		}
#endif // _WIN64

	}
	else
	{
		DbgPrint("New Win32pServiceTable Base is not valid\r\n");
	}
}


/************************************************************************
*  Name : APReloadWin32k
*  Param: VOID
*  Ret  : NTSTATUS
*  重载内核第一模块
************************************************************************/
NTSTATUS
APReloadWin32k()
{
	NTSTATUS    Status = STATUS_SUCCESS;

	if (g_ReloadWin32kImage == NULL)
	{
		PVOID          FileBuffer = NULL;
		PLDR_DATA_TABLE_ENTRY Win32kLdr = NULL;

		Win32kLdr = APGetDriverModuleLdr(L"win32k.sys", g_PsLoadedModuleList);

		Status = STATUS_UNSUCCESSFUL;

		// 2.读取第一模块文件到内存，按内存对齐格式完成PE的IAT，BaseReloc修复
		FileBuffer = APGetFileBuffer(&Win32kLdr->FullDllName);
		if (FileBuffer)
		{
			PIMAGE_DOS_HEADER DosHeader = NULL;
			PIMAGE_NT_HEADERS NtHeader = NULL;
			PIMAGE_SECTION_HEADER SectionHeader = NULL;

			DosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
			if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
			{
				NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)FileBuffer + DosHeader->e_lfanew);
				if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
				{
					g_ReloadWin32kImage = ExAllocatePool(NonPagedPool, NtHeader->OptionalHeader.SizeOfImage);
					if (g_ReloadWin32kImage)
					{
						DbgPrint("New Base::%p\r\n", g_ReloadWin32kImage);

						// 2.1.开始拷贝数据
						RtlZeroMemory(g_ReloadWin32kImage, NtHeader->OptionalHeader.SizeOfImage);
						// 2.1.1.拷贝头
						RtlCopyMemory(g_ReloadWin32kImage, FileBuffer, NtHeader->OptionalHeader.SizeOfHeaders);
						// 2.1.2.拷贝节区
						SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
						for (UINT16 i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
						{
							RtlCopyMemory((PUINT8)g_ReloadWin32kImage + SectionHeader[i].VirtualAddress,
								(PUINT8)FileBuffer + SectionHeader[i].PointerToRawData, SectionHeader[i].SizeOfRawData);
						}

						// 2.2.修复导入地址表
						APFixImportAddressTable(g_ReloadWin32kImage);

						// 2.3.修复重定向表
						APFixRelocBaseTable(g_ReloadWin32kImage, Win32kLdr->DllBase);

						// 2.4.修复SSDT
						APFixWin32pServiceTable(g_ReloadWin32kImage, Win32kLdr->DllBase);

						Status = STATUS_SUCCESS;
					}
					else
					{
						DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
					}
				}
				else
				{
					DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
				}
			}
			else
			{
				DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
			}
			ExFreePool(FileBuffer);
			FileBuffer = NULL;
		}

	}

	return Status;
}


/************************************************************************
*  Name : APEnumSssdtHook
*  Param: shi              
*  Param: SssdtFunctionCount
*  Ret  : NTSTATUS
*  重载Win32k 检查Sssdt Hook
************************************************************************/
NTSTATUS
APEnumSssdtHookByReloadWin32k(OUT PSSSDT_HOOK_INFORMATION shi, IN UINT32 SssdtFunctionCount)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;
	// 1.获得当前的SSSDT
	g_CurrentWin32pServiceTableAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentWin32pServiceTable();
	if (g_CurrentWin32pServiceTableAddress && MmIsAddressValid(g_CurrentWin32pServiceTableAddress))
	{
		// 2.Attach到gui进程
		PEPROCESS GuiEProcess = APGetGuiProcess();
		if (GuiEProcess &&MmIsAddressValid(GuiEProcess))
		{
			KAPC_STATE	ApcState = { 0 };

			// 转到目标进程空间上下背景文里
			KeStackAttachProcess(GuiEProcess, &ApcState);

			// 3.重载内核SSDT(得到原先的SSDT函数地址数组)
			Status = APReloadWin32k();
			if (NT_SUCCESS(Status))
			{
				// 3.对比Original&Current
				for (UINT32 i = 0; i < g_CurrentWin32pServiceTableAddress->Limit; i++)
				{
					if (SssdtFunctionCount >= shi->NumberOfSssdtFunctions)
					{
#ifdef _WIN64
						// 64位存储的是 偏移（高28位）
						//INT32 OriginalOffset = g_SssdtItem[i] >> 4;
						INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentWin32pServiceTableAddress->Base + i * 4)) >> 4;    // 带符号位的移位

						UINT64 CurrentSssdtFunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentWin32pServiceTableAddress->Base + CurrentOffset);
						UINT64 OriginalSssdtFunctionAddress = g_OriginalSssdtFunctionAddress[i];

#else
						// 32位存储的是 绝对地址
						UINT32 CurrentSssdtFunctionAddress = *(UINT32*)((UINT32)g_CurrentWin32pServiceTableAddress->Base + i * 4);
						UINT32 OriginalSssdtFunctionAddress = g_OriginalSssdtFunctionAddress[i];

#endif // _WIN64

						if (OriginalSssdtFunctionAddress != CurrentSssdtFunctionAddress)   // 表明被Hook了
						{
							shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].bHooked = TRUE;
						}
						else
						{
							shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].bHooked = FALSE;
						}
						shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].Ordinal = i;
						shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].CurrentAddress = CurrentSssdtFunctionAddress;
						shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].OriginalAddress = OriginalSssdtFunctionAddress;

						RtlStringCchCopyW(shi->SssdtHookEntry[shi->NumberOfSssdtFunctions].wzFunctionName, wcslen(g_SssdtFunctionName[i]) + 1, g_SssdtFunctionName[i]);

						Status = STATUS_SUCCESS;
					}
					else
					{
						Status = STATUS_BUFFER_TOO_SMALL;
					}
					shi->NumberOfSssdtFunctions++;
				}
			}
			else
			{
				DbgPrint("Reload Win32k & Sssdt Failed\r\n");
			}

			KeUnstackDetachProcess(&ApcState);
		}
	}
	else
	{
		DbgPrint("Get Current Sssdt Failed\r\n");
	}

	return Status;
}


/************************************************************************
*  Name : APEnumSssdtHook
*  Param: OutputBuffer            ring3内存
*  Param: OutputLength
*  Ret  : NTSTATUS
*  枚举进程模块
************************************************************************/
NTSTATUS
APEnumSssdtHook(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    SssdtFunctionCount = (OutputLength - sizeof(SSSDT_HOOK_INFORMATION)) / sizeof(SSSDT_HOOK_ENTRY_INFORMATION);

	PSSSDT_HOOK_INFORMATION shi = (PSSSDT_HOOK_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
	if (shi)
	{
		RtlZeroMemory(shi, OutputLength);

		Status = APEnumSssdtHookByReloadWin32k(shi, SssdtFunctionCount);
		if (NT_SUCCESS(Status))
		{
			if (SssdtFunctionCount >= shi->NumberOfSssdtFunctions)
			{
				RtlCopyMemory(OutputBuffer, shi, OutputLength);
				Status = STATUS_SUCCESS;
			}
			else
			{
				((PSSSDT_HOOK_INFORMATION)OutputBuffer)->NumberOfSssdtFunctions = shi->NumberOfSssdtFunctions;    // 让Ring3知道需要多少个
				Status = STATUS_BUFFER_TOO_SMALL;	// 给ring3返回内存不够的信息
			}
		}

		ExFreePool(shi);
		shi = NULL;
	}

	return Status;
}

