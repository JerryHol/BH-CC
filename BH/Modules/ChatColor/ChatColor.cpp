#include "ChatColor.h"
#include "../../BH.h"
#include "../../D2Ptrs.h"
#include "../../D2Stubs.h"

std::map<std::string, Toggle> ChatColor::Toggles;

Patch* UnicodeSupport2 = new Patch(Jump, D2LANG, { 0x8320, 0x82E0 }, (DWORD)D2Lang_Win2UnicodePatch, 5);
//{PatchJMP,    DLLOFFSET2(D2LANG, 0x6FC08320, 0x6FC082E0),      (DWORD)D2Lang_Win2UnicodePatch,          5 ,   &fLocalizationSupport},
//Ӷ��/����ͷ��ȼ���ʾ���л��������ʾ
Patch* petHead = new Patch(Call, D2CLIENT, { 0x5B582, 0xB254C }, (DWORD)DrawPetHeadPath_ASM, 7);
//{PatchCALL, DLLOFFSET2(D2CLIENT, 0x6FB0B582, 0x6FB39662), (DWORD)DrawPetHeadPath_ASM, 7, & fDefault},
Patch* partyHead = new Patch(Call, D2CLIENT, { 0x5BBE0, 0xB254C }, (DWORD)DrawPartyHeadPath_ASM, 6);
//{ PatchCALL,   DLLOFFSET2(D2CLIENT, 0x6FB0BBE0, 0x6FB39F90),    (DWORD)DrawPartyHeadPath_ASM,          6 ,   &fDefault },

void ChatColor::Init() {
	
}

void Print(DWORD color, char* format, ...) {
	va_list vaArgs;
	va_start(vaArgs, format);
	int len = _vscprintf(format, vaArgs) + 1;
	char* str = new char[len];
	vsprintf_s(str, len, format, vaArgs);
	va_end(vaArgs);

	wchar_t* wstr = new wchar_t[len];
	MultiByteToWideChar(CODE_PAGE, 0, str, -1, wstr, len);
	D2CLIENT_PrintGameString(wstr, color);
	delete[] wstr;

	delete[] str;
}

void ChatColor::OnGameJoin() {
	inGame = true;
}

void ChatColor::OnGameExit() {
	inGame = false;
}

void ChatColor::OnLoad() {
	UnicodeSupport2->Install();
	petHead->Install();
	partyHead->Install();
	LoadConfig();

}void ChatColor::OnUnload()
{
	UnicodeSupport2->Remove();
	petHead->Remove();
	partyHead->Remove();
}

void ChatColor::LoadConfig() {
	whisperColors.clear();

	BH::config->ReadAssoc("Whisper Color", whisperColors);
	BH::config->ReadToggle("Merc Protect", "None", true, Toggles["Merc Protect"]);  //Ӷ������
	BH::config->ReadToggle("Merc Boring", "None", true, Toggles["Merc Boring"]);  //Ӷ���²�
}

LPCSTR __fastcall D2Lang_Unicode2WinPatch(LPSTR lpWinStr, LPWSTR lpUnicodeStr, DWORD dwBufSize)
{
	WideCharToMultiByte(CP_ACP, 0, lpUnicodeStr, -1, lpWinStr, dwBufSize, NULL, NULL);
	return lpWinStr;
}

void UnicodeFix2(LPWSTR lpText)
{
	if (lpText) {
		size_t LEN = wcslen(lpText);
		for (size_t i = 0; i < LEN; i++)
		{
			if (lpText[i] == 0xf8f5) // Unicode 'y'
				lpText[i] = 0xff; // Ansi 'y'
		}
	}
}

int MyMultiByteToWideChar(
	UINT CodePage,         // code page
	DWORD dwFlags,         // character-type options
	LPCSTR lpMultiByteStr, // string to map
	int cbMultiByte,       // number of bytes in string
	LPWSTR lpWideCharStr,  // wide-character buffer
	int cchWideChar        // size of buffer
)
{
	int r = MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
	UnicodeFix2(lpWideCharStr);
	return r;
}

LPWSTR __fastcall D2Lang_Win2UnicodePatch(LPWSTR lpUnicodeStr, LPCSTR lpWinStr, DWORD dwBufSize)
{
	MyMultiByteToWideChar(CP_ACP, 0, lpWinStr, -1, lpUnicodeStr, dwBufSize);
	return lpUnicodeStr;
}


void ChatColor::UpdateInGame() {
	if ((*p_D2WIN_FirstControl) && inGame) inGame = false;
	else if (D2CLIENT_GetPlayerUnit() && !inGame) inGame = true;
}

void ChatColor::OnChatPacketRecv(BYTE* packet, bool* block) {
	// the game thread only checks if we've left the game every 10ms, so we update inGame here.
	UpdateInGame();	
	if (packet[1] == 0x0F && inGame) {
		unsigned int event_id = *(unsigned short int*)&packet[4];

		if (event_id == 4) {
			const char* from = (const char*)&packet[28];
			unsigned int fromLen = strlen(from);

			const char* message = (const char*)&packet[28 + fromLen + 1];
			unsigned int messageLen = strlen(message);

			bool replace = false;
			int color = 0;
			if (whisperColors.find(from) != whisperColors.end()) {
				replace = true;
				color = whisperColors[from];
			}
			UpdateInGame();
			if (replace && inGame) {
				*block = true;

				Print(color, "%s | %s", from, message);
			}
		}
	}
}
void AutoToBelt()
{
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return;

	//"hp", "mp", "rv"
		//ѭ�����ұ��������ҩ
	for (UnitAny* pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY || pItem->pItemData->ItemLocation == STORAGE_CUBE) {   //ֻȡ�����ͺ��������
			char* code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
			if (code[0] == 'h' && code[1] == 'p') {
				DWORD itemId = pItem->dwUnitId;  //��ҩ
				//��һ�������֪���ǲ����������ݰ�
				BYTE PacketData[5] = { 0x63, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = itemId;
				D2NET_SendPacket(5, 0, PacketData);
			}
			if (code[0] == 'm' && code[1] == 'p') {
				DWORD itemId = pItem->dwUnitId;  //��ҩ
				//��һ�������֪���ǲ����������ݰ�
				BYTE PacketData[5] = { 0x63, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = itemId;
				D2NET_SendPacket(5, 0, PacketData);
			}
			if (code[0] == 'r' && code[1] == 'v') {
				DWORD itemId = pItem->dwUnitId;  //��ҩ
				//��һ�������֪���ǲ����������ݰ�
				BYTE PacketData[5] = { 0x63, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = itemId;
				D2NET_SendPacket(5, 0, PacketData);
			}
		}
	}
}

//Ӷ���Զ���ҩ,ֻ֧�������ϵ�ҩ
void AutoMercDrink(double perHP) {
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return;
	if (perHP > 65) return;  //����65����ֵҲֱ����������
	//"hp", "mp", "rv"
		//ѭ�����ұ��������ҩ
	DWORD itemId = 0;
	char* code = "NULL";
	for (UnitAny* pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		if (pItem->pItemData->ItemLocation == STORAGE_NULL && pItem->pItemData->NodePage == NODEPAGE_BELTSLOTS) {   //ֻ�����������
			code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
			if (code[0] == 'h' && code[1] == 'p') {
				if (perHP <= 65) {  //�Ժ�ҩ
					itemId = pItem->dwUnitId;  //��ҩ
				}
			}
			if (code[0] == 'r' && code[1] == 'v') {
				if (perHP <= 35) {  //����ҩ
					itemId = pItem->dwUnitId;  //��ҩ
				}
			}
			if (itemId != 0) break;  //�ҵ��˾�ֱ���ж�ѭ��
		}
	}
	if (itemId == 0 && perHP <= 35) {
		if (ChatColor::Toggles["Merc Boring"].state)
			PrintText(Yellow, "�㶼ûҩ�����������һؼң���%.0f%%", perHP);
	}
	else {
		if (code[0] == 'h' && code[1] == 'p') {
			if (ChatColor::Toggles["Merc Boring"].state)
				PrintText(Red, "Ӷ����Ѫ��,�������,�ɱ�����%.0f%%", perHP);
		}
		else if (code[0] == 'r' && code[1] == 'v') {
			if (ChatColor::Toggles["Merc Boring"].state)
				PrintText(Purple, "Ӷ����Ѫ��,�Ϻ�����,�ɱ�����%.0f%%", perHP);
		}
	}
	if (itemId == 0) return;  //ûҩ�˻��ú�ֱ������
	BYTE PacketData[13] = { 0x26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	*reinterpret_cast<int*>(PacketData + 1) = itemId;
	*reinterpret_cast<int*>(PacketData + 5) = 1;  //�Ƿ��Ӷ����ҩ,1�Ǹ���0�ǲ���
	D2NET_SendPacket(13, 0, PacketData);
	//���껹Ҫ����ȥ
	Task::Enqueue([=]()->void {
		Sleep(1000);  //ͣ1�����Կ�
		AutoToBelt();
		});
}

//ͷ�����
void __fastcall DrawPetHeadPath(int xpos, UnitAny* pUnit) {

	wchar_t wszTemp[512];
	wsprintfW(wszTemp, L"%d", D2COMMON_GetUnitStat(pUnit, STAT_LEVEL, 0));
	//swprintf(wszTemp, L"%d��%f/%f", D2COMMON_GetUnitStat(pUnit, STAT_LEVEL, 0), hp,maxhp);

	//D2WIN_DrawText(1, wszTemp, xpos + 5, 57, 0);
	//DWORD dwOldFone = D2WIN_SetTextFont(1);   //��������
	D2WIN_DrawText(wszTemp, xpos + 5, 57, White, 0);
	//D2WIN_DrawText(wszTemp, xpos + 700, 570, White, 0);
	//D2WIN_SetTextFont(dwOldFone);
	bool test1 = ChatColor::Toggles["Merc Protect"].state;
	if (test1) {
		//������Ӷ���Զ���ҩ
		DWORD mHP = D2COMMON_GetUnitStat(pUnit, STAT_HP, 0);
		if (mHP > 0x8000) {  //���˵��merc��Ѫ�����˱仯
			double maxhp = (double)(D2COMMON_GetUnitStat(pUnit, STAT_MAXHP, 0) >> 8);
			double hp = (double)(mHP >> 8);
			double perHP = (hp / maxhp) * 100.0;

			if (perHP < mercLastHP)
			{
				//PrintText(White, "Ӷ��Ѫ���٣�%.0f%%", perHP);
				//��ʼ��ҩ
				AutoMercDrink(perHP);
			}
			mercLastHP = perHP;
		}
	}
}

void __declspec(naked) DrawPetHeadPath_ASM()
{
	//ecx  xpos
	//eax  ypos
	//ebx  pPet
	__asm {
		push esi

		mov edx, ebx
		call DrawPetHeadPath

		pop esi
		//org
		mov[esp + 0x56], 0
		ret
	}
}


void __fastcall DrawPartyHeadPath(int xpos, RosterUnit* pRosterUnit) {

	wchar_t wszTemp[512];

	//if (tShowPartyLevel.isOn) {
	wsprintfW(wszTemp, L"%d", pRosterUnit->wLevel);
	//DrawD2Text(1, wszTemp, xpos + 5, 57, 0);
	//DWORD dwOldFone = D2WIN_SetTextFont(1);   //��������
	D2WIN_DrawText(wszTemp, xpos + 5, 57, White, 0);
	//D2WIN_SetTextFont(dwOldFone);
	//}

	//if (tShowPartyPosition.isOn) {  //�����ſ��������Ȳ���
	//	wsprintfW(wszTemp, L"%d", pRosterUnit->dwLevelNo);
	//	DrawCenterText(1, wszTemp, xpos + 20, 15, 4, 1, 1);
	//}
}

void __declspec(naked) DrawPartyHeadPath_ASM()
{
	//[ebx]  xpos
	//eax  ypos
	//[esp+0C]  pRosterUnit
	__asm {
		mov ecx, dword ptr[ebx]
		mov edx, dword ptr[esp + 0xC]

		push ebx
		push edi
		push eax

		call DrawPartyHeadPath

		pop eax
		pop edi
		pop ebx

		mov ecx, dword ptr[ebx]
		mov edx, dword ptr[esp + 0xC]
		ret
	}
}