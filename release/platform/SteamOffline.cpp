#include "steam/steam_api.h"
#include "steam/isteamnetworkingutils.h"
#include <cstring>
class OfflineISteamUser final : public ISteamUser {
public:
 HSteamUser GetHSteamUser() override { return {}; }
 bool BLoggedOn() override { return {}; }
 // A valid ID (CSteamID::IsValid): Client sends to the local host only while the host's ID is valid; with the raw ID 1
 // no pickup, loot or other interaction reached it.
 CSteamID GetSteamID() override { return CSteamID(1, k_EUniversePublic, k_EAccountTypeIndividual); }
 int InitiateGameConnection( void *pAuthBlob, int cbMaxAuthBlob, CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer, bool bSecure ) override { return {}; }
 void TerminateGameConnection( uint32 unIPServer, uint16 usPortServer ) override {  }
 void TrackAppUsageEvent( CGameID gameID, int eAppUsageEvent, const char *pchExtraInfo) override {  }
 bool GetUserDataFolder( char *pchBuffer, int cubBuffer ) override { return {}; }
 void StartVoiceRecording( ) override {  }
 void StopVoiceRecording( ) override {  }
 EVoiceResult GetAvailableVoice( uint32 *pcbCompressed, uint32 *pcbUncompressed_Deprecated, uint32 nUncompressedVoiceDesiredSampleRate_Deprecated) override { return {}; }
 EVoiceResult GetVoice( bool bWantCompressed, void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten, bool bWantUncompressed_Deprecated, void *pUncompressedDestBuffer_Deprecated, uint32 cbUncompressedDestBufferSize_Deprecated, uint32 *nUncompressBytesWritten_Deprecated, uint32 nUncompressedVoiceDesiredSampleRate_Deprecated) override { return {}; }
 EVoiceResult DecompressVoice( const void *pCompressed, uint32 cbCompressed, void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten, uint32 nDesiredSampleRate ) override { return {}; }
 uint32 GetVoiceOptimalSampleRate() override { return {}; }
 HAuthTicket GetAuthSessionTicket( void *pTicket, int cbMaxTicket, uint32 *pcbTicket ) override { return {}; }
 EBeginAuthSessionResult BeginAuthSession( const void *pAuthTicket, int cbAuthTicket, CSteamID steamID ) override { return {}; }
 void EndAuthSession( CSteamID steamID ) override {  }
 void CancelAuthTicket( HAuthTicket hAuthTicket ) override {  }
 EUserHasLicenseForAppResult UserHasLicenseForApp( CSteamID steamID, AppId_t appID ) override { return {}; }
 bool BIsBehindNAT() override { return {}; }
 void AdvertiseGame( CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer ) override {  }
 SteamAPICall_t RequestEncryptedAppTicket( void *pDataToInclude, int cbDataToInclude ) override { return {}; }
 bool GetEncryptedAppTicket( void *pTicket, int cbMaxTicket, uint32 *pcbTicket ) override { return {}; }
 int GetGameBadgeLevel( int nSeries, bool bFoil ) override { return {}; }
 int GetPlayerSteamLevel() override { return {}; }
 SteamAPICall_t RequestStoreAuthURL( const char *pchRedirectURL ) override { return {}; }
 bool BIsPhoneVerified() override { return {}; }
 bool BIsTwoFactorEnabled() override { return {}; }
 bool BIsPhoneIdentifying() override { return {}; }
 bool BIsPhoneRequiringVerification() override { return {}; }
 SteamAPICall_t GetMarketEligibility() override { return {}; }
 SteamAPICall_t GetDurationControl() override { return {}; }
};
static OfflineISteamUser offlineISteamUser;
class OfflineISteamFriends final : public ISteamFriends {
public:
 const char *GetPersonaName() override { return "Local player"; }
 SteamAPICall_t SetPersonaName( const char *pchPersonaName ) override { return {}; }
 EPersonaState GetPersonaState() override { return {}; }
 int GetFriendCount( int iFriendFlags ) override { return {}; }
 CSteamID GetFriendByIndex( int iFriend, int iFriendFlags ) override { return {}; }
 EFriendRelationship GetFriendRelationship( CSteamID steamIDFriend ) override { return {}; }
 EPersonaState GetFriendPersonaState( CSteamID steamIDFriend ) override { return {}; }
 const char *GetFriendPersonaName( CSteamID steamIDFriend ) override { return "Local player"; }
 bool GetFriendGamePlayed( CSteamID steamIDFriend, STEAM_OUT_STRUCT() FriendGameInfo_t *pFriendGameInfo ) override { return {}; }
 const char *GetFriendPersonaNameHistory( CSteamID steamIDFriend, int iPersonaName ) override { return ""; }
 int GetFriendSteamLevel( CSteamID steamIDFriend ) override { return {}; }
 const char *GetPlayerNickname( CSteamID steamIDPlayer ) override { return ""; }
 int GetFriendsGroupCount() override { return {}; }
 FriendsGroupID_t GetFriendsGroupIDByIndex( int iFG ) override { return {}; }
 const char *GetFriendsGroupName( FriendsGroupID_t friendsGroupID ) override { return ""; }
 int GetFriendsGroupMembersCount( FriendsGroupID_t friendsGroupID ) override { return {}; }
 void GetFriendsGroupMembersList( FriendsGroupID_t friendsGroupID, STEAM_OUT_ARRAY_CALL(nMembersCount, GetFriendsGroupMembersCount, friendsGroupID ) CSteamID *pOutSteamIDMembers, int nMembersCount ) override {  }
 bool HasFriend( CSteamID steamIDFriend, int iFriendFlags ) override { return {}; }
 int GetClanCount() override { return {}; }
 CSteamID GetClanByIndex( int iClan ) override { return {}; }
 const char *GetClanName( CSteamID steamIDClan ) override { return ""; }
 const char *GetClanTag( CSteamID steamIDClan ) override { return ""; }
 bool GetClanActivityCounts( CSteamID steamIDClan, int *pnOnline, int *pnInGame, int *pnChatting ) override { return {}; }
 SteamAPICall_t DownloadClanActivityCounts( STEAM_ARRAY_COUNT(cClansToRequest) CSteamID *psteamIDClans, int cClansToRequest ) override { return {}; }
 int GetFriendCountFromSource( CSteamID steamIDSource ) override { return {}; }
 CSteamID GetFriendFromSourceByIndex( CSteamID steamIDSource, int iFriend ) override { return {}; }
 bool IsUserInSource( CSteamID steamIDUser, CSteamID steamIDSource ) override { return {}; }
 void SetInGameVoiceSpeaking( CSteamID steamIDUser, bool bSpeaking ) override {  }
 void ActivateGameOverlay( const char *pchDialog ) override {  }
 void ActivateGameOverlayToUser( const char *pchDialog, CSteamID steamID ) override {  }
 void ActivateGameOverlayToWebPage( const char *pchURL, EActivateGameOverlayToWebPageMode eMode) override {  }
 void ActivateGameOverlayToStore( AppId_t nAppID, EOverlayToStoreFlag eFlag ) override {  }
 void SetPlayedWith( CSteamID steamIDUserPlayedWith ) override {  }
 void ActivateGameOverlayInviteDialog( CSteamID steamIDLobby ) override {  }
 int GetSmallFriendAvatar( CSteamID steamIDFriend ) override { return {}; }
 int GetMediumFriendAvatar( CSteamID steamIDFriend ) override { return {}; }
 int GetLargeFriendAvatar( CSteamID steamIDFriend ) override { return {}; }
 bool RequestUserInformation( CSteamID steamIDUser, bool bRequireNameOnly ) override { return {}; }
 SteamAPICall_t RequestClanOfficerList( CSteamID steamIDClan ) override { return {}; }
 CSteamID GetClanOwner( CSteamID steamIDClan ) override { return {}; }
 int GetClanOfficerCount( CSteamID steamIDClan ) override { return {}; }
 CSteamID GetClanOfficerByIndex( CSteamID steamIDClan, int iOfficer ) override { return {}; }
 uint32 GetUserRestrictions() override { return {}; }
 bool SetRichPresence( const char *pchKey, const char *pchValue ) override { return {}; }
 void ClearRichPresence() override {  }
 const char *GetFriendRichPresence( CSteamID steamIDFriend, const char *pchKey ) override { return ""; }
 int GetFriendRichPresenceKeyCount( CSteamID steamIDFriend ) override { return {}; }
 const char *GetFriendRichPresenceKeyByIndex( CSteamID steamIDFriend, int iKey ) override { return ""; }
 void RequestFriendRichPresence( CSteamID steamIDFriend ) override {  }
 bool InviteUserToGame( CSteamID steamIDFriend, const char *pchConnectString ) override { return {}; }
 int GetCoplayFriendCount() override { return {}; }
 CSteamID GetCoplayFriend( int iCoplayFriend ) override { return {}; }
 int GetFriendCoplayTime( CSteamID steamIDFriend ) override { return {}; }
 AppId_t GetFriendCoplayGame( CSteamID steamIDFriend ) override { return {}; }
 SteamAPICall_t JoinClanChatRoom( CSteamID steamIDClan ) override { return {}; }
 bool LeaveClanChatRoom( CSteamID steamIDClan ) override { return {}; }
 int GetClanChatMemberCount( CSteamID steamIDClan ) override { return {}; }
 CSteamID GetChatMemberByIndex( CSteamID steamIDClan, int iUser ) override { return {}; }
 bool SendClanChatMessage( CSteamID steamIDClanChat, const char *pchText ) override { return {}; }
 int GetClanChatMessage( CSteamID steamIDClanChat, int iMessage, void *prgchText, int cchTextMax, EChatEntryType *peChatEntryType, STEAM_OUT_STRUCT() CSteamID *psteamidChatter ) override { return {}; }
 bool IsClanChatAdmin( CSteamID steamIDClanChat, CSteamID steamIDUser ) override { return {}; }
 bool IsClanChatWindowOpenInSteam( CSteamID steamIDClanChat ) override { return {}; }
 bool OpenClanChatWindowInSteam( CSteamID steamIDClanChat ) override { return {}; }
 bool CloseClanChatWindowInSteam( CSteamID steamIDClanChat ) override { return {}; }
 bool SetListenForFriendsMessages( bool bInterceptEnabled ) override { return {}; }
 bool ReplyToFriendMessage( CSteamID steamIDFriend, const char *pchMsgToSend ) override { return {}; }
 int GetFriendMessage( CSteamID steamIDFriend, int iMessageID, void *pvData, int cubData, EChatEntryType *peChatEntryType ) override { return {}; }
 SteamAPICall_t GetFollowerCount( CSteamID steamID ) override { return {}; }
 SteamAPICall_t IsFollowing( CSteamID steamID ) override { return {}; }
 SteamAPICall_t EnumerateFollowingList( uint32 unStartIndex ) override { return {}; }
 bool IsClanPublic( CSteamID steamIDClan ) override { return {}; }
 bool IsClanOfficialGameGroup( CSteamID steamIDClan ) override { return {}; }
 int GetNumChatsWithUnreadPriorityMessages() override { return {}; }
};
static OfflineISteamFriends offlineISteamFriends;
class OfflineISteamNetworking final : public ISteamNetworking {
public:
 bool SendP2PPacket( CSteamID steamIDRemote, const void *pubData, uint32 cubData, EP2PSend eP2PSendType, int nChannel) override { return {}; }
 bool IsP2PPacketAvailable( uint32 *pcubMsgSize, int nChannel) override { return {}; }
 bool ReadP2PPacket( void *pubDest, uint32 cubDest, uint32 *pcubMsgSize, CSteamID *psteamIDRemote, int nChannel) override { return {}; }
 bool AcceptP2PSessionWithUser( CSteamID steamIDRemote ) override { return {}; }
 bool CloseP2PSessionWithUser( CSteamID steamIDRemote ) override { return {}; }
 bool CloseP2PChannelWithUser( CSteamID steamIDRemote, int nChannel ) override { return {}; }
 bool GetP2PSessionState( CSteamID steamIDRemote, P2PSessionState_t *pConnectionState ) override { return {}; }
 bool AllowP2PPacketRelay( bool bAllow ) override { return {}; }
 SNetListenSocket_t CreateListenSocket( int nVirtualP2PPort, uint32 nIP, uint16 nPort, bool bAllowUseOfPacketRelay ) override { return {}; }
 SNetSocket_t CreateP2PConnectionSocket( CSteamID steamIDTarget, int nVirtualPort, int nTimeoutSec, bool bAllowUseOfPacketRelay ) override { return {}; }
 SNetSocket_t CreateConnectionSocket( uint32 nIP, uint16 nPort, int nTimeoutSec ) override { return {}; }
 bool DestroySocket( SNetSocket_t hSocket, bool bNotifyRemoteEnd ) override { return {}; }
 bool DestroyListenSocket( SNetListenSocket_t hSocket, bool bNotifyRemoteEnd ) override { return {}; }
 bool SendDataOnSocket( SNetSocket_t hSocket, void *pubData, uint32 cubData, bool bReliable ) override { return {}; }
 bool IsDataAvailableOnSocket( SNetSocket_t hSocket, uint32 *pcubMsgSize ) override { return {}; }
 bool RetrieveDataFromSocket( SNetSocket_t hSocket, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize ) override { return {}; }
 bool IsDataAvailable( SNetListenSocket_t hListenSocket, uint32 *pcubMsgSize, SNetSocket_t *phSocket ) override { return {}; }
 bool RetrieveData( SNetListenSocket_t hListenSocket, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize, SNetSocket_t *phSocket ) override { return {}; }
 bool GetSocketInfo( SNetSocket_t hSocket, CSteamID *pSteamIDRemote, int *peSocketStatus, uint32 *punIPRemote, uint16 *punPortRemote ) override { return {}; }
 bool GetListenSocketInfo( SNetListenSocket_t hListenSocket, uint32 *pnIP, uint16 *pnPort ) override { return {}; }
 ESNetSocketConnectionType GetSocketConnectionType( SNetSocket_t hSocket ) override { return {}; }
 int GetMaxPacketSize( SNetSocket_t hSocket ) override { return {}; }
};
static OfflineISteamNetworking offlineISteamNetworking;
class OfflineISteamUtils final : public ISteamUtils {
public:
 uint32 GetSecondsSinceAppActive() override { return {}; }
 uint32 GetSecondsSinceComputerActive() override { return {}; }
 EUniverse GetConnectedUniverse() override { return {}; }
 uint32 GetServerRealTime() override { return {}; }
 const char *GetIPCountry() override { return ""; }
 bool GetImageSize( int iImage, uint32 *pnWidth, uint32 *pnHeight ) override { return {}; }
 bool GetImageRGBA( int iImage, uint8 *pubDest, int nDestBufferSize ) override { return {}; }
 bool GetCSERIPPort( uint32 *unIP, uint16 *usPort ) override { return {}; }
 uint8 GetCurrentBatteryPower() override { return {}; }
 uint32 GetAppID() override { return 1128000; }
 void SetOverlayNotificationPosition( ENotificationPosition eNotificationPosition ) override {  }
 bool IsAPICallCompleted( SteamAPICall_t hSteamAPICall, bool *pbFailed ) override { return {}; }
 ESteamAPICallFailure GetAPICallFailureReason( SteamAPICall_t hSteamAPICall ) override { return {}; }
 bool GetAPICallResult( SteamAPICall_t hSteamAPICall, void *pCallback, int cubCallback, int iCallbackExpected, bool *pbFailed ) override { return {}; }
 void RunFrame() override {  }
 uint32 GetIPCCallCount() override { return {}; }
 void SetWarningMessageHook( SteamAPIWarningMessageHook_t pFunction ) override {  }
 bool IsOverlayEnabled() override { return {}; }
 bool BOverlayNeedsPresent() override { return {}; }
 SteamAPICall_t CheckFileSignature( const char *szFileName ) override { return {}; }
 bool ShowGamepadTextInput( EGamepadTextInputMode eInputMode, EGamepadTextInputLineMode eLineInputMode, const char *pchDescription, uint32 unCharMax, const char *pchExistingText ) override { return {}; }
 uint32 GetEnteredGamepadTextLength() override { return {}; }
 bool GetEnteredGamepadTextInput( char *pchText, uint32 cchText ) override { return {}; }
 const char *GetSteamUILanguage() override { return ""; }
 bool IsSteamRunningInVR() override { return {}; }
 void SetOverlayNotificationInset( int nHorizontalInset, int nVerticalInset ) override {  }
 bool IsSteamInBigPictureMode() override { return {}; }
 void StartVRDashboard() override {  }
 bool IsVRHeadsetStreamingEnabled() override { return {}; }
 void SetVRHeadsetStreamingEnabled( bool bEnabled ) override {  }
 bool IsSteamChinaLauncher() override { return {}; }
 bool InitFilterText() override { return {}; }
 int FilterText( char* pchOutFilteredText, uint32 nByteSizeOutFilteredText, const char * pchInputMessage, bool bLegalOnly ) override { return {}; }
};
static OfflineISteamUtils offlineISteamUtils;
class OfflineISteamNetworkingUtils final : public ISteamNetworkingUtils {
public:
 ESteamNetworkingAvailability GetRelayNetworkStatus( SteamRelayNetworkStatus_t *pDetails ) override { return {}; }
 float GetLocalPingLocation( SteamNetworkPingLocation_t &result ) override { return {}; }
 int EstimatePingTimeBetweenTwoLocations( const SteamNetworkPingLocation_t &location1, const SteamNetworkPingLocation_t &location2 ) override { return {}; }
 int EstimatePingTimeFromLocalHost( const SteamNetworkPingLocation_t &remoteLocation ) override { return {}; }
 void ConvertPingLocationToString( const SteamNetworkPingLocation_t &location, char *pszBuf, int cchBufSize ) override {  }
 bool ParsePingLocationString( const char *pszString, SteamNetworkPingLocation_t &result ) override { return {}; }
 bool CheckPingDataUpToDate( float flMaxAgeSeconds ) override { return {}; }
 int GetPingToDataCenter( SteamNetworkingPOPID popID, SteamNetworkingPOPID *pViaRelayPoP ) override { return {}; }
 int GetDirectPingToPOP( SteamNetworkingPOPID popID ) override { return {}; }
 int GetPOPCount() override { return {}; }
 int GetPOPList( SteamNetworkingPOPID *list, int nListSz ) override { return {}; }
 SteamNetworkingMicroseconds GetLocalTimestamp() override { return {}; }
 void SetDebugOutputFunction( ESteamNetworkingSocketsDebugOutputType eDetailLevel, FSteamNetworkingSocketsDebugOutput pfnFunc ) override {  }
 bool SetConfigValue( ESteamNetworkingConfigValue eValue, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj, ESteamNetworkingConfigDataType eDataType, const void *pArg ) override { return {}; }
 ESteamNetworkingGetConfigValueResult GetConfigValue( ESteamNetworkingConfigValue eValue, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj, ESteamNetworkingConfigDataType *pOutDataType, void *pResult, size_t *cbResult ) override { return {}; }
 bool GetConfigValueInfo( ESteamNetworkingConfigValue eValue, const char **pOutName, ESteamNetworkingConfigDataType *pOutDataType, ESteamNetworkingConfigScope *pOutScope, ESteamNetworkingConfigValue *pOutNextValue ) override { return {}; }
 ESteamNetworkingConfigValue GetFirstConfigValue() override { return {}; }
 void SteamNetworkingIPAddr_ToString( const SteamNetworkingIPAddr &addr, char *buf, size_t cbBuf, bool bWithPort ) override {  }
 bool SteamNetworkingIPAddr_ParseString( SteamNetworkingIPAddr *pAddr, const char *pszStr ) override { return {}; }
 void SteamNetworkingIdentity_ToString( const SteamNetworkingIdentity &identity, char *buf, size_t cbBuf ) override {  }
 bool SteamNetworkingIdentity_ParseString( SteamNetworkingIdentity *pIdentity, const char *pszStr ) override { return {}; }
};
static OfflineISteamNetworkingUtils offlineISteamNetworkingUtils;
extern "C" {
bool SteamAPI_Init() { return true; }
void SteamAPI_Shutdown() {}
void SteamAPI_RunCallbacks() {}
HSteamUser SteamAPI_GetHSteamUser() { return 1; }
HSteamPipe SteamAPI_GetHSteamPipe() { return 1; }
void SteamAPI_RegisterCallback(CCallbackBase*, int) {}
void SteamAPI_UnregisterCallback(CCallbackBase*) {}
void SteamAPI_RegisterCallResult(CCallbackBase*, SteamAPICall_t) {}
void SteamAPI_UnregisterCallResult(CCallbackBase*, SteamAPICall_t) {}
void *SteamInternal_ContextInit(void *data) {
 auto slots = static_cast<void**>(data);
 reinterpret_cast<void(*)(void*)>(slots[0])(&slots[2]);
 return &slots[2];
}
void *SteamInternal_FindOrCreateUserInterface(HSteamUser, const char *version) {
 if (std::strncmp(version, "SteamUser", 9) == 0) return &offlineISteamUser;
 if (std::strncmp(version, "SteamFriends", 12) == 0) return &offlineISteamFriends;
 if (std::strncmp(version, "SteamNetworking", 15) == 0) return &offlineISteamNetworking;
 if (std::strncmp(version, "SteamUtils", 10) == 0) return &offlineISteamUtils;
 if (std::strncmp(version, "SteamNetworkingUtils", 20) == 0) return &offlineISteamNetworkingUtils;
 return nullptr;
}
}

ISteamNetworkingUtils::~ISteamNetworkingUtils() = default;
