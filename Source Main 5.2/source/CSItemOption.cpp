/*+++++++++++++++++++++++++++++++++++++
    INCLUDE.
+++++++++++++++++++++++++++++++++++++*/
#include "stdafx.h"
#include "ZzzOpenglUtil.h"
#include "zzzInfomation.h"
#include "zzzBmd.h"
#include "zzztexture.h"
#include "zzzCharacter.h"
#include "zzzscene.h"
#include "zzzInterface.h"
#include "zzzinventory.h"
#include "CSItemOption.h"
#include "CharacterManager.h"
#include "UIControls.h"
#include "NewUISystem.h"
#include "SkillManager.h"

extern	wchar_t TextList[50][100];
extern	int  TextListColor[50];
extern	int  TextBold[50];

static  CSItemOption csItemOption; // do not delete, required for singleton initialization.

constexpr int MAX_EQUIPPED_SETS = MAX_EQUIPMENT_INDEX - 2; // No Wings and Helper slots

struct ITEM_SET_OPTION_FILE
{
    char	strSetName[MAX_ITEM_SET_NAME];
    BYTE	byStandardOption[MAX_ITEM_SET_STANDARD_OPTION_COUNT][MAX_ITEM_SETS_PER_ITEM];
    BYTE	byStandardOptionValue[MAX_ITEM_SET_STANDARD_OPTION_COUNT][MAX_ITEM_SETS_PER_ITEM];
    BYTE	byExtOption[MAX_ITEM_SETS_PER_ITEM];
    BYTE	byExtOptionValue[MAX_ITEM_SETS_PER_ITEM];
    BYTE	byOptionCount;
    BYTE	byFullOption[MAX_ITEM_SET_FULL_OPTION_COUNT];
    BYTE	byFullOptionValue[MAX_ITEM_SET_FULL_OPTION_COUNT];
    BYTE	byRequireClass[MAX_CLASS];
};

static BYTE bBuxCode[3] = { 0xfc,0xcf,0xab };

static void BuxConvert(BYTE* Buffer, int Size)
{
    for (int i = 0; i < Size; i++)
        Buffer[i] ^= bBuxCode[i % 3];
}

bool CSItemOption::OpenItemSetScript(bool bTestServer)
{
    std::wstring strFileName = L"";
    std::wstring strTest = (bTestServer) ? L"Test" : L"";

    strFileName = L"Data\\Local\\ItemSetType" + strTest + L".bmd";
    if (!OpenItemSetType(strFileName.c_str()))		return false;

    strFileName = L"Data\\Local\\" + g_strSelectedML + L"\\ItemSetOption" + strTest + L"_" + g_strSelectedML + L".bmd";
    if (!OpenItemSetOption(strFileName.c_str()))	 	return false;
    return true;
}

bool CSItemOption::OpenItemSetType(const wchar_t* filename)
{
    FILE* fp = _wfopen(filename, L"rb");
    if (fp != nullptr)
    {
        int Size = sizeof(ITEM_SET_TYPE);
        BYTE* Buffer = new BYTE[Size * MAX_ITEM];
        fread(Buffer, Size * MAX_ITEM, 1, fp);

        DWORD dwCheckSum;
        fread(&dwCheckSum, sizeof(DWORD), 1, fp);
        fclose(fp);

        if (dwCheckSum != GenerateCheckSum2(Buffer, Size * MAX_ITEM, 0xE5F1))
        {
            wchar_t Text[256];
            swprintf(Text, L"%s - File corrupted.", filename);
            g_ErrorReport.Write(Text);
            MessageBox(g_hWnd, Text, nullptr, MB_OK);
            SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        }
        else
        {
            BYTE* pSeek = Buffer;
            for (int i = 0; i < MAX_ITEM; i++)
            {
                BuxConvert(pSeek, Size);
                memcpy(&m_ItemSetType[i], pSeek, Size);

                pSeek += Size;
            }
        }
        delete[] Buffer;
    }
    else
    {
        wchar_t Text[256];
        swprintf(Text, L"%s - File not exist.", filename);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
    }

    return true;
}

bool CSItemOption::OpenItemSetOption(const wchar_t* filename)
{
    FILE* fp = _wfopen(filename, L"rb");
    if (fp != nullptr)
    {
        const int Size = sizeof(ITEM_SET_OPTION_FILE);
        BYTE* Buffer = new BYTE[Size * MAX_SET_OPTION];
        fread(Buffer, Size * MAX_SET_OPTION, 1, fp);

        DWORD dwCheckSum;
        fread(&dwCheckSum, sizeof(DWORD), 1, fp);
        fclose(fp);

        if (dwCheckSum != GenerateCheckSum2(Buffer, Size * MAX_SET_OPTION, 0xA2F1))
        {
            wchar_t Text[256];
            swprintf(Text, L"%s - File corrupted.", filename);
            g_ErrorReport.Write(Text);
            MessageBox(g_hWnd, Text, nullptr, MB_OK);
            SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        }
        else
        {
            BYTE* pSeek = Buffer;
            for (int i = 0; i < MAX_SET_OPTION; i++)
            {
                BuxConvert(pSeek, Size);

                ITEM_SET_OPTION_FILE current{ };
                memcpy(&current, pSeek, Size);
                const auto target = &m_ItemSetOption[i];

                CMultiLanguage::ConvertFromUtf8(target->strSetName, current.strSetName);
                target->byOptionCount = current.byOptionCount;
                for (int setIndex = 0; setIndex < MAX_ITEM_SETS_PER_ITEM; ++setIndex)
                {
                    target->byExtOption[setIndex] = current.byExtOption[setIndex];
                    target->byExtOptionValue[setIndex] = current.byExtOptionValue[setIndex];
                    for (int optionIndex = 0; optionIndex < MAX_ITEM_SET_STANDARD_OPTION_COUNT; ++optionIndex)
                    {
                        target->byStandardOption[optionIndex][setIndex] = current.byStandardOption[optionIndex][setIndex];
                        target->byStandardOptionValue[optionIndex][setIndex] = current.byStandardOptionValue[optionIndex][setIndex];
                    }
                }

                for (int optionIndex = 0; optionIndex < MAX_ITEM_SET_FULL_OPTION_COUNT; ++optionIndex)
                {
                    target->byFullOption[optionIndex] = current.byFullOption[optionIndex];
                    target->byFullOptionValue[optionIndex] = current.byFullOptionValue[optionIndex];
                }

                for (int i = 0; i < MAX_CLASS; i++)
                {
                    target->byRequireClass[i] = current.byRequireClass[i];
                }

                pSeek += Size;
            }
        }
        delete[] Buffer;
    }
    else
    {
        wchar_t Text[256];
        swprintf(Text, L"%s - File does not exist.", filename);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
    }

    return true;
}

bool CSItemOption::IsDisableSkill(int Type, int Energy, int Charisma)
{
    int SkillEnergy = 20 + SkillAttribute[Type].Energy * (SkillAttribute[Type].Level) * 4 / 100;

    if (Type == AT_SKILL_SUMMON_EXPLOSION || Type == AT_SKILL_SUMMON_REQUIEM)
    {
        SkillEnergy = 20 + SkillAttribute[Type].Energy * (SkillAttribute[Type].Level) * 3 / 100;
    }

    if (gCharacterManager.GetBaseClass(Hero->Class) == CLASS_KNIGHT)
    {
        SkillEnergy = 10 + SkillAttribute[Type].Energy * (SkillAttribute[Type].Level) * 4 / 100;
    }

    switch (Type)
    {
    case 17:SkillEnergy = 0; break;
    case 30:SkillEnergy = 30; break;
    case 31:SkillEnergy = 60; break;
    case 32:SkillEnergy = 90; break;
    case 33:SkillEnergy = 130; break;
    case 34:SkillEnergy = 170; break;
    case 35:SkillEnergy = 210; break;
    case 36:SkillEnergy = 300; break;
    case 37:SkillEnergy = 500; break;
    case 60:SkillEnergy = 15; break;
    case AT_SKILL_ASHAKE_UP:
    case AT_SKILL_ASHAKE_UP + 1:
    case AT_SKILL_ASHAKE_UP + 2:
    case AT_SKILL_ASHAKE_UP + 3:
    case AT_SKILL_ASHAKE_UP + 4:
    case AT_SKILL_DARK_HORSE:    SkillEnergy = 0; break;
    case AT_PET_COMMAND_DEFAULT: SkillEnergy = 0; break;
    case AT_PET_COMMAND_RANDOM:  SkillEnergy = 0; break;
    case AT_PET_COMMAND_OWNER:   SkillEnergy = 0; break;
    case AT_PET_COMMAND_TARGET:  SkillEnergy = 0; break;
    case AT_SKILL_PLASMA_STORM_FENRIR: SkillEnergy = 0; break;
    case AT_SKILL_INFINITY_ARROW: SkillEnergy = 0; break;
    case AT_SKILL_BLOW_OF_DESTRUCTION: SkillEnergy = 0; break;
    case AT_SKILL_RECOVER:
    case AT_SKILL_GAOTIC:
    case AT_SKILL_MULTI_SHOT:
    case AT_SKILL_FIRE_SCREAM_UP:
    case AT_SKILL_FIRE_SCREAM_UP + 1:
    case AT_SKILL_FIRE_SCREAM_UP + 2:
    case AT_SKILL_FIRE_SCREAM_UP + 3:
    case AT_SKILL_FIRE_SCREAM_UP + 4:
    case AT_SKILL_DARK_SCREAM:
        SkillEnergy = 0;
        break;

    case AT_SKILL_EXPLODE:
        SkillEnergy = 0;
        break;
    }

    if (Type >= AT_SKILL_STUN && Type <= AT_SKILL_REMOVAL_BUFF)
    {
        SkillEnergy = 0;
    }
    else
        if ((Type >= 18 && Type <= 23) || (Type >= 41 && Type <= 43) || (Type >= 47 && Type <= 49) || Type == 24 || Type == 51 || Type == 52 || Type == 55 || Type == 56)
        {
            SkillEnergy = 0;
        }
        else if (Type == 44 || Type == 45 || Type == 46 || Type == 57 || Type == 73 || Type == 74)
        {
            SkillEnergy = 0;
        }

    if (Charisma > 0)
    {
        int SkillCharisma = SkillAttribute[Type].Charisma;
        if (Charisma < SkillCharisma)
        {
            return true;
        }
    }

    if (Energy < SkillEnergy)
    {
        return true;
    }

    return false;
}

BYTE CSItemOption::IsChangeSetItem(const int Type, const int SubType = -1)
{
    ITEM_SET_TYPE& itemSType = m_ItemSetType[Type];

    if (SubType == -1)
    {
        if (itemSType.byOption[0] == 255 && itemSType.byOption[1] == 255)
            return 0;
        return 255;
    }
    else
    {
        if (itemSType.byOption[SubType] == 255)
            return 0;

        return SubType + 1;
    }
}

WORD CSItemOption::GetMixItemLevel(const int Type)
{
    if (Type < 0) return 0;

    WORD MixLevel = 0;
    const ITEM_SET_TYPE& itemSType = m_ItemSetType[Type];

    MixLevel = MAKEWORD(itemSType.byMixItemLevel[0], itemSType.byMixItemLevel[1]);

    return MixLevel;
}

bool CSItemOption::GetSetItemName(wchar_t* strName, const int iType, const int setType)
{
    int setItemType = (setType % 0x04);

    if (setItemType > 0)
    {
        const ITEM_SET_TYPE& itemSType = m_ItemSetType[iType];
        if (itemSType.byOption[setItemType - 1] != 255 && itemSType.byOption[setItemType - 1] != 0)
        {
            const ITEM_SET_OPTION& itemOption = m_ItemSetOption[itemSType.byOption[setItemType - 1]];

            
            memcpy(strName, itemOption.strSetName, sizeof itemOption.strSetName);

            const int length = wcslen(strName);
            strName[length] = ' ';
            strName[length + 1] = 0;
            return true;
        }
    }
    return false;
}


void CSItemOption::checkItemType(SET_SEARCH_RESULT* optionList, const int iType, const int setType)
{
    const int setItemType = (setType % 0x04);

    if (setItemType <= 0)
    {
        return;
    }

    const auto setTypeIndex = static_cast<BYTE>(setItemType - 1);

    const ITEM_SET_TYPE& itemSetType = m_ItemSetType[iType];
    const auto itemSetNumber = itemSetType.byOption[setTypeIndex];

    if (itemSetNumber != 255 && itemSetNumber != 0)
    {
        // add set item to list
        for (int i = 0; i < MAX_EQUIPPED_SETS; ++i)
        {
            const auto current = &optionList[i];
            if (current->SetNumber == 0)
            {
                // The set wasn't found in another item yet, so add it
                current->SetNumber = itemSetNumber;
                current->ItemCount++;
                current->SetTypeIndex = setTypeIndex;
                break;
            }

            if (current->SetNumber == itemSetNumber)
            {
                // The set wasn't found in another item yet, so add it
                current->ItemCount++;
                current->SetTypeIndex = setTypeIndex;
                break;
            }
        }
    }
}

void CSItemOption::calcSetOptionList(SET_SEARCH_RESULT* optionList)
{
    int Class = gCharacterManager.GetBaseClass(Hero->Class);
    int ExClass = gCharacterManager.IsSecondClass(Hero->Class);

    BYTE    bySetOptionListTmp[MAX_ITEM_SETS_PER_ITEM][MAX_OPTIONS_PER_ITEM_SET];
    BYTE    bySetOptionListTmp2[MAX_ITEM_SETS_PER_ITEM][MAX_OPTIONS_PER_ITEM_SET];
    int     iSetOptionListValue[MAX_ITEM_SETS_PER_ITEM][MAX_OPTIONS_PER_ITEM_SET];

    unsigned int arruiSetItemTypeSequence[MAX_ITEM_SETS_PER_ITEM][MAX_OPTIONS_PER_ITEM_SET] = {};

    BYTE    optionCount[2] = { 0, 0 };  //
    BYTE    setType = 0;

    m_bySameSetItem = 0;
    for (auto& i : m_strSetName)
    {
        i[0] = 0;
    }

    for (auto& i : m_bySetOptionIndex)
    {
        i = 0;
    }

    m_bySetOptionANum = 0;
    m_bySetOptionBNum = 0;
    Hero->ExtendState = 0;

    unsigned int iSetItemTypeSequence = 0, iCurrentSetItemTypeSequence = 0;

    m_mapEquippedSetItemName.clear();
    m_mapEquippedSetItemSequence.clear();

    for (int i = 0; i < MAX_EQUIPPED_SETS; i += 3)
    {
        const auto current = &optionList[i];
        if (current->SetNumber == 0)
        {
            // we can already stop here
            break;
        }

        assert(current->SetNumber < MAX_SET_OPTION);

        if (current->ItemCount >= 2)
        {
            const int standardCount = min(current->ItemCount - 1, MAX_ITEM_SET_STANDARD_OPTION_COUNT);
            setType = current->SetTypeIndex;
            assert(setType < MAX_ITEM_SETS_PER_ITEM);

            ITEM_SET_OPTION& itemOption = m_ItemSetOption[current->SetNumber];

            BYTE RequireClass = 0;
            if (itemOption.byRequireClass[0] == 1 && Class == CLASS_WIZARD) RequireClass = 1;
            if (itemOption.byRequireClass[0] == 2 && Class == CLASS_WIZARD && ExClass) RequireClass = 1;
            if (itemOption.byRequireClass[1] == 1 && Class == CLASS_KNIGHT) RequireClass = 1;
            if (itemOption.byRequireClass[1] == 2 && Class == CLASS_KNIGHT && ExClass) RequireClass = 1;
            if (itemOption.byRequireClass[2] == 1 && Class == CLASS_ELF) RequireClass = 1;
            if (itemOption.byRequireClass[2] == 2 && Class == CLASS_ELF && ExClass) RequireClass = 1;
            if (itemOption.byRequireClass[3] == 1 && Class == CLASS_DARK) RequireClass = 1;
            if (itemOption.byRequireClass[3] == 1 && Class == CLASS_DARK && ExClass) RequireClass = 1;
            if (itemOption.byRequireClass[4] == 1 && Class == CLASS_DARK_LORD) RequireClass = 1;
            if (itemOption.byRequireClass[4] == 1 && Class == CLASS_DARK_LORD && ExClass) RequireClass = 1;
            if (itemOption.byRequireClass[5] == 1 && Class == CLASS_SUMMONER) RequireClass = 1;
            if (itemOption.byRequireClass[5] == 1 && Class == CLASS_SUMMONER && ExClass) RequireClass = 1;
            if (itemOption.byRequireClass[6] == 1 && Class == CLASS_RAGEFIGHTER) RequireClass = 1;
            if (itemOption.byRequireClass[6] == 1 && Class == CLASS_RAGEFIGHTER && ExClass) RequireClass = 1;

            m_bySetOptionIndex[setType] = optionList[i].SetNumber;

            if (m_strSetName[setType][0] != 0
                && wcscmp(m_strSetName[setType], itemOption.strSetName) != 0)
            {
                if (m_strSetName[0][0] == 0)
                    wcscpy(m_strSetName[0], itemOption.strSetName);
                else
                    wcscpy(m_strSetName[1], itemOption.strSetName);
                m_bySameSetItem = current->ItemCount;
            }
            else
            {
                wcscpy(m_strSetName[setType], itemOption.strSetName);
            }

            bool	bFind = false;
            for (m_iterESIN = m_mapEquippedSetItemName.begin(); m_iterESIN != m_mapEquippedSetItemName.end(); ++m_iterESIN)
            {
                std::wstring strCur = m_iterESIN->second;

                if (wcscmp(itemOption.strSetName, strCur.c_str()) == 0)
                {
                    bFind = true;
                    break;
                }
            }

            if (false == bFind)
            {
                iCurrentSetItemTypeSequence = iSetItemTypeSequence++;
                m_mapEquippedSetItemName.insert(std::pair<int, std::wstring>(iCurrentSetItemTypeSequence, itemOption.strSetName));
            }

            constexpr BYTE EMPTY_OPTION = 0xFF;
            BYTE option[MAX_ITEM_SETS_PER_ITEM];
            int  value[MAX_ITEM_SETS_PER_ITEM];
            for (int j = 0; j < current->ItemCount; ++j)
            {
                option[0] = EMPTY_OPTION;
                option[1] = EMPTY_OPTION;
                if (j < standardCount)
                {
                    option[0] = itemOption.byStandardOption[j][0];
                    value[0] = itemOption.byStandardOptionValue[j][0];
                    option[1] = itemOption.byStandardOption[j][1];
                    value[1] = itemOption.byStandardOptionValue[j][1];
                }
                else if (j < (current->ItemCount - standardCount))
                {
                    option[0] = itemOption.byExtOption[j];
                    value[0] = itemOption.byExtOptionValue[j];
                }

                if (option[0] != EMPTY_OPTION)
                {
                    if (option[0] < MASTERY_OPTION)
                    {
                        arruiSetItemTypeSequence[setType][optionCount[setType]] = iCurrentSetItemTypeSequence;
                        bySetOptionListTmp[setType][optionCount[setType]] = option[0];
                        bySetOptionListTmp2[setType][optionCount[setType]] = RequireClass;
                        iSetOptionListValue[setType][optionCount[setType]] = value[0];
                        optionCount[setType]++;
                    }
                    else
                    {
                        if (itemOption.byRequireClass[Class] && ExClass >= itemOption.byRequireClass[Class] - 1)
                        {
                            arruiSetItemTypeSequence[setType][optionCount[setType]] = iCurrentSetItemTypeSequence;
                            bySetOptionListTmp[setType][optionCount[setType]] = option[0];
                            bySetOptionListTmp2[setType][optionCount[setType]] = RequireClass;
                            iSetOptionListValue[setType][optionCount[setType]] = value[0];
                            optionCount[setType]++;
                        }
                    }
                }
                if (option[1] != EMPTY_OPTION)
                {
                    if (option[1] < MASTERY_OPTION)
                    {
                        arruiSetItemTypeSequence[setType][optionCount[setType]] = iCurrentSetItemTypeSequence;
                        bySetOptionListTmp[setType][optionCount[setType]] = option[1];
                        bySetOptionListTmp2[setType][optionCount[setType]] = RequireClass;
                        iSetOptionListValue[setType][optionCount[setType]] = value[1];
                        optionCount[setType]++;
                        assert(optionCount[setType] < MAX_OPTIONS_PER_ITEM_SET);
                    }
                    else
                    {
                        if (itemOption.byRequireClass[Class] && ExClass >= itemOption.byRequireClass[Class] - 1)
                        {
                            arruiSetItemTypeSequence[setType][optionCount[setType]] = iCurrentSetItemTypeSequence;
                            bySetOptionListTmp[setType][optionCount[setType]] = option[1];
                            bySetOptionListTmp2[setType][optionCount[setType]] = RequireClass;
                            iSetOptionListValue[setType][optionCount[setType]] = value[1];
                            optionCount[setType]++;
                            assert(optionCount[setType] < MAX_OPTIONS_PER_ITEM_SET);
                        }
                    }
                }
            }

            if (current->ItemCount >= itemOption.byOptionCount - 2)
            {
                for (int j = 0; j < MAX_ITEM_SET_FULL_OPTION_COUNT; ++j)
                {
                    option[0] = itemOption.byFullOption[j];
                    value[0] = itemOption.byFullOptionValue[j];
                    if (option[0] != EMPTY_OPTION)
                    {
                        if (option[0] < MASTERY_OPTION)
                        {
                            arruiSetItemTypeSequence[setType][optionCount[setType]] = iCurrentSetItemTypeSequence;
                            bySetOptionListTmp[setType][optionCount[setType]] = option[0];
                            bySetOptionListTmp2[setType][optionCount[setType]] = RequireClass;
                            iSetOptionListValue[setType][optionCount[setType]] = value[0];
                            optionCount[setType]++;
                            assert(optionCount[setType] < MAX_OPTIONS_PER_ITEM_SET);

                            if (m_bySameSetItem != 0) m_bySameSetItem++;
                        }
                        else
                        {
                            if (itemOption.byRequireClass[Class] && ExClass >= itemOption.byRequireClass[Class] - 1)
                            {
                                arruiSetItemTypeSequence[setType][optionCount[setType]] = iCurrentSetItemTypeSequence;
                                bySetOptionListTmp[setType][optionCount[setType]] = option[0];
                                bySetOptionListTmp2[setType][optionCount[setType]] = RequireClass;
                                iSetOptionListValue[setType][optionCount[setType]] = value[0];
                                optionCount[setType]++;
                                assert(optionCount[setType] < MAX_OPTIONS_PER_ITEM_SET);

                                if (m_bySameSetItem != 0) m_bySameSetItem++;
                            }
                        }
                    }
                }

                Hero->ExtendState = 1;
            }
        }
    }

    for (int i = 0; i < MAX_ITEM_SETS_PER_ITEM; ++i)
    {
        for (int j = 0; j < optionCount[i]; ++j)
        {
            m_mapEquippedSetItemSequence.insert(std::pair<BYTE, int>((i * optionCount[0]) + j, arruiSetItemTypeSequence[i][j]));
            m_bySetOptionList[(i * optionCount[0]) + j][0] = bySetOptionListTmp[i][j];
            m_bySetOptionList[(i * optionCount[0]) + j][1] = bySetOptionListTmp2[i][j];
            m_iSetOptionListValue[(i * optionCount[0]) + j][0] = iSetOptionListValue[i][j];
            m_iSetOptionListValue[(i * optionCount[0]) + j][1] = iSetOptionListValue[i][j];
        }
    }
    m_bySetOptionANum = optionCount[0];
    m_bySetOptionBNum = optionCount[1];
}

void CSItemOption::getExplainText(wchar_t* text, const BYTE option, const BYTE value, const BYTE SetIndex)
{
    switch (option + AT_SET_OPTION_IMPROVE_STRENGTH)
    {
    case AT_SET_OPTION_IMPROVE_MAGIC_POWER:
        swprintf(text, GlobalText[632], value);
        break;

    case AT_SET_OPTION_IMPROVE_STRENGTH:
    case AT_SET_OPTION_IMPROVE_DEXTERITY:
    case AT_SET_OPTION_IMPROVE_ENERGY:
    case AT_SET_OPTION_IMPROVE_VITALITY:
    case AT_SET_OPTION_IMPROVE_CHARISMA:
    case AT_SET_OPTION_IMPROVE_ATTACK_MIN:
    case AT_SET_OPTION_IMPROVE_ATTACK_MAX:
        swprintf(text, GlobalText[950 + option], value);
        break;

    case AT_SET_OPTION_IMPROVE_DAMAGE:
    case AT_SET_OPTION_IMPROVE_ATTACKING_PERCENT:
    case AT_SET_OPTION_IMPROVE_DEFENCE:
    case AT_SET_OPTION_IMPROVE_MAX_LIFE:
    case AT_SET_OPTION_IMPROVE_MAX_MANA:
    case AT_SET_OPTION_IMPROVE_MAX_AG:
    case AT_SET_OPTION_IMPROVE_ADD_AG:
    case AT_SET_OPTION_IMPROVE_CRITICAL_DAMAGE_PERCENT:
    case AT_SET_OPTION_IMPROVE_CRITICAL_DAMAGE:
    case AT_SET_OPTION_IMPROVE_EXCELLENT_DAMAGE_PERCENT:
    case AT_SET_OPTION_IMPROVE_EXCELLENT_DAMAGE:
    case AT_SET_OPTION_IMPROVE_SKILL_ATTACK:
    case AT_SET_OPTION_DOUBLE_DAMAGE:
        swprintf(text, GlobalText[949 + option], value);
        break;

    case AT_SET_OPTION_DISABLE_DEFENCE:
        swprintf(text, GlobalText[970], value);
        break;

    case AT_SET_OPTION_TWO_HAND_SWORD_IMPROVE_DAMAGE:
        swprintf(text, GlobalText[983], value);
        break;

    case AT_SET_OPTION_IMPROVE_SHIELD_DEFENCE:
        swprintf(text, GlobalText[984], value);
        break;

    case AT_SET_OPTION_IMPROVE_ATTACK_1:
    case AT_SET_OPTION_IMPROVE_ATTACK_2:
    case AT_SET_OPTION_IMPROVE_MAGIC:
        //	case AT_SET_OPTION_IMPROVE_DEFENCE_1:
        //	case AT_SET_OPTION_IMPROVE_DEFENCE_2:
    case AT_SET_OPTION_IMPROVE_DEFENCE_3:
    case AT_SET_OPTION_IMPROVE_DEFENCE_4:
    case AT_SET_OPTION_FIRE_MASTERY:
    case AT_SET_OPTION_ICE_MASTERY:
    case AT_SET_OPTION_THUNDER_MASTERY:
    case AT_SET_OPTION_POSION_MASTERY:
    case AT_SET_OPTION_WATER_MASTERY:
    case AT_SET_OPTION_WIND_MASTERY:
    case AT_SET_OPTION_EARTH_MASTERY:
        swprintf(text, GlobalText[971 + (option + AT_SET_OPTION_IMPROVE_STRENGTH - AT_SET_OPTION_IMPROVE_ATTACK_2)], value);
        break;
    }
}

void CSItemOption::PlusSpecial(WORD* Value, int Special)
{
    Special -= AT_SET_OPTION_IMPROVE_STRENGTH;
    int optionValue = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] == Special && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue += m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
        }
    }

    if (optionValue)
    {
        *Value += optionValue;
    }
}

void CSItemOption::PlusSpecialPercent(WORD* Value, int Special)
{
    Special -= AT_SET_OPTION_IMPROVE_STRENGTH;
    int optionValue = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] == Special && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue += m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
        }
    }

    if (optionValue)
    {
        *Value += ((*Value) * optionValue) / 100;
    }
}

void CSItemOption::PlusSpecialLevel(WORD* Value, const WORD SrcValue, int Special)
{
    Special -= AT_SET_OPTION_IMPROVE_STRENGTH;
    int optionValue = 0;
    int count = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] == Special && m_bySetOptionList[i][1] != 0 && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue = m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
            count++;
        }
    }

    if (optionValue)
    {
        optionValue = SrcValue * optionValue / 100;
        *Value += (optionValue * count);
    }
}

void CSItemOption::PlusMastery(int* Value, const BYTE MasteryType)
{
    int optionValue = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] >= MASTERY_OPTION && (m_bySetOptionList[i][0] - MASTERY_OPTION - 5) == MasteryType && m_bySetOptionList[i][1] != 0 && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue += m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
        }
    }

    if (optionValue)
    {
        *Value += optionValue;
    }
}

void CSItemOption::MinusSpecialPercent(int* Value, int Special)
{
    Special -= AT_SET_OPTION_IMPROVE_STRENGTH;
    int optionValue = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] == Special && m_bySetOptionList[i][1] != 0 && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue += m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
        }
    }

    if (optionValue)
    {
        *Value -= *Value * optionValue / 100;
    }
}

void CSItemOption::GetSpecial(WORD* Value, int Special)
{
    Special -= AT_SET_OPTION_IMPROVE_STRENGTH;
    int optionValue = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] == Special && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue += m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
        }
    }

    if (optionValue)
    {
        *Value += optionValue;
    }
}

void	CSItemOption::GetSpecialPercent(WORD* Value, int Special)
{
    Special -= AT_SET_OPTION_IMPROVE_STRENGTH;
    int optionValue = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] == Special && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue += m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
        }
    }

    if (optionValue)
    {
        *Value += *Value * optionValue / 100;
    }
}

void	CSItemOption::GetSpecialLevel(WORD* Value, const WORD SrcValue, int Special)
{
    Special -= AT_SET_OPTION_IMPROVE_STRENGTH;
    int optionValue = 0;
    int count = 0;
    for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
    {
        if (m_bySetOptionList[i][0] == Special && m_bySetOptionList[i][1] != 0 && m_bySetOptionListOnOff[i] == 0)
        {
            optionValue = m_iSetOptionListValue[i][0];
            m_bySetOptionListOnOff[i] = 1;
            count++;
        }
    }

    if (optionValue)
    {
        optionValue = SrcValue * optionValue / 100;
        *Value += (optionValue * count);
    }
}

int CSItemOption::GetDefaultOptionValue(ITEM* ip, WORD* Value)
{
    if (ip->Type > MAX_ITEM)
    {
        return -1;
    }

    *Value = ((ip->ExtOption >> 2) % 0x04);

    ITEM_ATTRIBUTE* p = &ItemAttribute[ip->Type];

    return p->AttType;
}

bool CSItemOption::GetDefaultOptionText(const ITEM* ip, wchar_t* Text)
{
    if (ip->Type > MAX_ITEM)
    {
        return false;
    }

    if (((ip->ExtOption >> 2) % 0x04) <= 0) return false;

    switch (ItemAttribute[ip->Type].AttType)
    {
    case SET_OPTION_STRENGTH:
        swprintf(Text, GlobalText[950], ((ip->ExtOption >> 2) % 0x04) * 5);
        break;

    case SET_OPTION_DEXTERITY:
        swprintf(Text, GlobalText[951], ((ip->ExtOption >> 2) % 0x04) * 5);
        break;

    case SET_OPTION_ENERGY:
        swprintf(Text, GlobalText[952], ((ip->ExtOption >> 2) % 0x04) * 5);
        break;

    case SET_OPTION_VITALITY:
        swprintf(Text, GlobalText[953], ((ip->ExtOption >> 2) % 0x04) * 5);
        break;

    default:
        return false;
    }
    return true;
}

bool CSItemOption::Special_Option_Check(int Kind)
{
    int i, j;
    for (i = 0; i < 2; i++)
    {
        ITEM* item = nullptr;
        item = &CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT + i];
        if (item == nullptr || item->Type <= -1)
            continue;

        if (Kind == 0)
        {
            for (j = 0; j < item->SpecialNum; j++)
            {
                if (item->Special[j] == AT_SKILL_ICE_BLADE)
                    return true;
            }
        }
        else
            if (Kind == 1)
            {
                for (j = 0; j < item->SpecialNum; j++)
                {
                    if (item->Special[j] == AT_SKILL_CROSSBOW)
                        return true;
                }
            }
    }
    return false;
}

int CSItemOption::RenderDefaultOptionText(const ITEM* ip, int TextNum)
{
    int TNum = TextNum;
    if (GetDefaultOptionText(ip, TextList[TNum]))
    {
        TextListColor[TNum] = TEXT_COLOR_BLUE;
        TNum++;

        if ((ip->Type >= ITEM_HELPER + 8 && ip->Type <= ITEM_HELPER + 9) || (ip->Type >= ITEM_HELPER + 12 && ip->Type <= ITEM_HELPER + 13) || (ip->Type >= ITEM_HELPER + 21 && ip->Type <= ITEM_HELPER + 27))
        {
            swprintf(TextList[TNum], GlobalText[1165]);
            TextListColor[TNum] = TEXT_COLOR_BLUE;
            TNum++;
        }
    }

    return TNum;
}

void CSItemOption::getAllAddState(WORD* Strength, WORD* Dexterity, WORD* Energy, WORD* Vitality, WORD* Charisma)
{
    for (int i = EQUIPMENT_WEAPON_RIGHT; i < MAX_EQUIPMENT; ++i)
    {
        ITEM* item = &CharacterMachine->Equipment[i];

        if (item->Durability <= 0)
        {
            continue;
        }

        WORD Result = 0;
        switch (GetDefaultOptionValue(item, &Result))
        {
        case SET_OPTION_STRENGTH:
            *Strength += Result * 5;
            break;

        case SET_OPTION_DEXTERITY:
            *Dexterity += Result * 5;
            break;

        case SET_OPTION_ENERGY:
            *Energy += Result * 5;
            break;

        case SET_OPTION_VITALITY:
            *Vitality += Result * 5;
            break;
        }
    }

    GetSpecial(Strength, AT_SET_OPTION_IMPROVE_STRENGTH);
    GetSpecial(Dexterity, AT_SET_OPTION_IMPROVE_DEXTERITY);
    GetSpecial(Energy, AT_SET_OPTION_IMPROVE_ENERGY);
    GetSpecial(Vitality, AT_SET_OPTION_IMPROVE_VITALITY);
    GetSpecial(Charisma, AT_SET_OPTION_IMPROVE_CHARISMA);
}

void CSItemOption::getAllAddStateOnlyAddValue(WORD* AddStrength, WORD* AddDexterity, WORD* AddEnergy, WORD* AddVitality, WORD* AddCharisma)
{
    *AddStrength = *AddDexterity = *AddEnergy = *AddVitality = *AddCharisma = 0;
    memset(m_bySetOptionListOnOff, 0, sizeof m_bySetOptionListOnOff);

    getAllAddState(AddStrength, AddDexterity, AddEnergy, AddVitality, AddCharisma);
}

void CSItemOption::getAllAddOptionStatesbyCompare(WORD* Strength, WORD* Dexterity, WORD* Energy, WORD* Vitality, WORD* Charisma, WORD iCompareStrength, WORD iCompareDexterity, WORD iCompareEnergy, WORD iCompareVitality, WORD iCompareCharisma)
{
    for (int i = EQUIPMENT_WEAPON_RIGHT; i < MAX_EQUIPMENT; ++i)
    {
        ITEM* item = &CharacterMachine->Equipment[i];

        if (item->RequireStrength > iCompareStrength ||
            item->RequireDexterity > iCompareDexterity ||
            item->RequireEnergy > iCompareEnergy)
        {
            continue;
        }

        if (item->Durability <= 0)
        {
            continue;
        }

        WORD Result = 0;
        switch (GetDefaultOptionValue(item, &Result))
        {
        case SET_OPTION_STRENGTH:
            *Strength += Result * 5;
            break;

        case SET_OPTION_DEXTERITY:
            *Dexterity += Result * 5;
            break;

        case SET_OPTION_ENERGY:
            *Energy += Result * 5;
            break;

        case SET_OPTION_VITALITY:
            *Vitality += Result * 5;
            break;
        }
    }

    memset(m_bySetOptionListOnOff, 0, sizeof m_bySetOptionListOnOff);

    GetSpecial(Strength, AT_SET_OPTION_IMPROVE_STRENGTH);
    GetSpecial(Dexterity, AT_SET_OPTION_IMPROVE_DEXTERITY);
    GetSpecial(Energy, AT_SET_OPTION_IMPROVE_ENERGY);
    GetSpecial(Vitality, AT_SET_OPTION_IMPROVE_VITALITY);
    GetSpecial(Charisma, AT_SET_OPTION_IMPROVE_CHARISMA);
}

void CSItemOption::UpdateCount_SetOptionPerEquippedSetItem(const SET_SEARCH_RESULT* byOptionList, int* arLimitSetItemOptionCount, ITEM* ItemsEquipment)
{
    for (int iE = 0; iE < MAX_EQUIPMENT_INDEX; ++iE)
    {
        int& iCurCount = arLimitSetItemOptionCount[iE];
        ITEM& CurItem = ItemsEquipment[iE];

        iCurCount = GetCurrentTypeSetitemCount(CurItem, byOptionList);
    }
}

int CSItemOption::Search_From_EquippedSetItemNameMapTable(wchar_t* szSetItemname)
{
    int		iSizeFindName = wcslen(szSetItemname);

    for (m_iterESIN = m_mapEquippedSetItemName.begin(); m_iterESIN != m_mapEquippedSetItemName.end(); ++m_iterESIN)
    {
        std::wstring	strCur;

        strCur = m_iterESIN->second;
        int iSizeCurName = strCur.size();

        if (!wcsncmp(szSetItemname, strCur.c_str(), iSizeFindName)
            && !wcsncmp(szSetItemname, strCur.c_str(), iSizeCurName))
        {
            return m_iterESIN->first;
        }
    }
    return -1;
}

int CSItemOption::GetCurrentTypeSetitemCount(const ITEM& CurItem_, const SET_SEARCH_RESULT* byOptionList)
{
    const BYTE bySetType = CurItem_.ExtOption;

    const int setItemType = (bySetType % 0x04);

    const ITEM_SET_TYPE& itemSType = m_ItemSetType[CurItem_.Type];

    for (int i = 0; i < MAX_EQUIPPED_SETS; ++i)
    {
        if (byOptionList[i].SetNumber == itemSType.byOption[(setItemType - 1)])
        {
            const int iEquippedCount = byOptionList[i + 1].ItemCount;
            const ITEM_SET_OPTION& itemOption = m_ItemSetOption[byOptionList[i].SetNumber];
            if (iEquippedCount >= itemOption.byOptionCount - 1)
            {
                return 255;
            }

            return iEquippedCount;
        }
    }

    return 0;
}

void CSItemOption::CheckItemSetOptions()
{
    SET_SEARCH_RESULT byOptionList[MAX_EQUIPPED_SETS] = { };
    const ITEM* itemRight = nullptr;

    ZeroMemory(m_bySetOptionList, sizeof(BYTE) * MAX_OPTIONS_PER_ITEM_SET * MAX_ITEM_SETS_PER_ITEM);

    for (int i = 0; i < MAX_EQUIPMENT_INDEX; ++i)
    {
        if (i == EQUIPMENT_WING || i == EQUIPMENT_HELPER)
        {
            continue;
        }

        ITEM* ip = &CharacterMachine->Equipment[i];

        if (ip->Durability <= 0)
        {
            continue;
        }

        if ((i == EQUIPMENT_WEAPON_LEFT || i == EQUIPMENT_RING_LEFT)
            && itemRight != nullptr && itemRight->Type == ip->Type && (itemRight->ExtOption % 0x04) == (ip->ExtOption % 0x04))
        {
            continue;
        }

        if (ip->Type > -1)
        {
            checkItemType(byOptionList, ip->Type, ip->ExtOption);
        }

        if (i == EQUIPMENT_WEAPON_RIGHT || i == EQUIPMENT_RING_RIGHT)
        {
            itemRight = ip;
        }
    }

    calcSetOptionList(byOptionList);
    getAllAddStateOnlyAddValue(&CharacterAttribute->AddStrength, &CharacterAttribute->AddDexterity, &CharacterAttribute->AddEnergy, &CharacterAttribute->AddVitality, &CharacterAttribute->AddCharisma);

    WORD AllStrength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
    WORD AllDexterity = CharacterAttribute->Dexterity + CharacterAttribute->AddDexterity;
    WORD AllEnergy = CharacterAttribute->Energy + CharacterAttribute->AddEnergy;
    WORD AllVitality = CharacterAttribute->Vitality + CharacterAttribute->AddVitality;
    WORD AllCharisma = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;
    WORD AllLevel = CharacterAttribute->Level;

    
    ZeroMemory(byOptionList, sizeof byOptionList);

    memset(m_bySetOptionList, 0xFF, sizeof m_bySetOptionList);

    for (int i = 0; i < MAX_EQUIPMENT_INDEX; ++i)
    {
        if (i == EQUIPMENT_WING || i == EQUIPMENT_HELPER)
        {
            continue;
        }

        ITEM* ip = &CharacterMachine->Equipment[i];

        if (ip->RequireDexterity > AllDexterity || ip->RequireEnergy > AllEnergy || ip->RequireStrength > AllStrength || ip->RequireLevel > AllLevel || ip->RequireCharisma > AllCharisma || ip->Durability <= 0 || (IsRequireEquipItem(ip) == false)) {
            continue;
        }

        if (((i == EQUIPMENT_WEAPON_LEFT || i == EQUIPMENT_RING_LEFT)
            && itemRight != nullptr && itemRight->Type == ip->Type && (itemRight->ExtOption % 0x04) == (ip->ExtOption % 0x04)))
        {
            continue;
        }

        if (ip->Type > -1)
        {
            checkItemType(byOptionList, ip->Type, ip->ExtOption);
        }

        if (i == EQUIPMENT_WEAPON_RIGHT || i == EQUIPMENT_RING_RIGHT)
        {
            itemRight = ip;
        }
    }

    UpdateCount_SetOptionPerEquippedSetItem(byOptionList, m_arLimitSetItemOptionCount, CharacterMachine->Equipment);
    calcSetOptionList(byOptionList);
}

void CSItemOption::MoveSetOptionList(const int StartX, const int StartY)
{
    int x, y, Width, Height;

    Width = 162; Height = 20; x = StartX + 14; y = StartY + 22;
    if (MouseX >= x && MouseX < x + Width && MouseY >= y && MouseY < y + Height)
    {
        m_bViewOptionList = true;

        MouseLButtonPush = false;
        MouseUpdateTime = 0;
        MouseUpdateTimeMax = 6;
    }
}

void CSItemOption::RenderSetOptionButton(const int StartX, const int StartY)
{
    float x, y, Width, Height;
    wchar_t  Text[100];

    Width = 162.f; Height = 20.f; x = (float)StartX + 14; y = (float)StartY + 22;
    RenderBitmap(BITMAP_INTERFACE_EX + 21, x, y, Width, Height, 0.f, 0.f, Width / 256.f, Height / 32.f);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(0, 0, 0, 255);
    g_pRenderText->SetBgColor(100, 0, 0, 0);
    swprintf(Text, L"[%s]", GlobalText[989]);
    g_pRenderText->RenderText(StartX + 96, (int)(y + 3), Text, 0, 0, RT3_WRITE_CENTER);

    g_pRenderText->SetTextColor(0xffffffff);
    if (m_bySetOptionANum > 0 || m_bySetOptionBNum > 0)
        g_pRenderText->SetTextColor(255, 204, 25, 255);
    else
        g_pRenderText->SetTextColor(128, 128, 128, 255);
    g_pRenderText->RenderText(StartX + 95, (int)(y + 2), Text, 0, 0, RT3_WRITE_CENTER);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
}

void CSItemOption::RenderSetOptionList(const int StartX, const int StartY)
{
    if (m_bViewOptionList && (m_bySetOptionANum > 0 || m_bySetOptionBNum > 0))
    {
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->SetBgColor(100, 0, 0, 0);

        int PosX, PosY;

        PosX = StartX + 95;
        PosY = StartY + 40;

        BYTE TextNum = 0;
        BYTE SkipNum = 0;
        BYTE setIndex = 0;

        swprintf(TextList[TextNum], L"\n"); TextListColor[TextNum] = 0; TextBold[TextNum] = false; TextNum++; SkipNum++;
        swprintf(TextList[TextNum], L"\n"); TextListColor[TextNum] = 0; TextBold[TextNum] = false; TextNum++; SkipNum++;
        swprintf(TextList[TextNum], L"\n"); TextListColor[TextNum] = 0; TextBold[TextNum] = false; TextNum++; SkipNum++;

        int		iCurSetItemTypeSequence = 0, iCurSetItemType = -1;

        for (int i = 0; i < m_bySetOptionANum + m_bySetOptionBNum; ++i)
        {
            std::wstring	strCurrent;

            m_iterESIS = m_mapEquippedSetItemSequence.find(i);

            if (m_mapEquippedSetItemSequence.end() != m_iterESIS)
            {
                iCurSetItemTypeSequence = m_iterESIS->second;
            }
            else
            {
                iCurSetItemTypeSequence = -1;
            }

            if (iCurSetItemType != iCurSetItemTypeSequence)
            {
                iCurSetItemType = iCurSetItemTypeSequence;

                m_iterESIN = m_mapEquippedSetItemName.find(iCurSetItemTypeSequence);

                if (m_iterESIN != m_mapEquippedSetItemName.end())
                {
                    strCurrent = m_iterESIN->second;

                    swprintf(TextList[TextNum], L"%s %s", strCurrent.c_str(), GlobalText[1089]);
                    TextListColor[TextNum] = 3;
                    TextBold[TextNum] = true;
                    TextNum++;
                }
            }
            getExplainText(TextList[TextNum], m_bySetOptionList[i][0], m_iSetOptionListValue[i][0], setIndex);
            if (m_bySetOptionList[i][0] >= AT_SET_OPTION_IMPROVE_ATTACK_1 && m_bySetOptionList[i][1] == 0)
                TextListColor[TextNum] = 2;
            else
                TextListColor[TextNum] = 1;
            TextBold[TextNum] = false;
            TextNum++;
        }

        RenderTipTextList(PosX, PosY, TextNum, 120, RT3_SORT_CENTER);
        m_bViewOptionList = false;
    }
}

void CSItemOption::CheckRenderOptionHelper(const wchar_t* FilterName)
{
    wchar_t Name[256];

    if (FilterName[0] != '/') return;

    auto Length1 = wcslen(FilterName);
    for (int i = 0; i < MAX_SET_OPTION; ++i)
    {
        ITEM_SET_OPTION& setOption = m_ItemSetOption[i];
        if (setOption.byOptionCount < 255)
        {
            swprintf(Name, L"/%s", setOption.strSetName);

            auto Length2 = wcslen(Name);

            m_byRenderOptionList = 0;
            if (wcsncmp(FilterName, Name, Length1) == NULL && wcsncmp(FilterName, Name, Length2) == NULL)
            {
                g_pNewUISystem->Hide(SEASON3B::INTERFACE_ITEM_EXPLANATION);
                g_pNewUISystem->Hide(SEASON3B::INTERFACE_HELP);
                g_pNewUISystem->Show(SEASON3B::INTERFACE_SETITEM_EXPLANATION);

                m_byRenderOptionList = i + 1;
                return;
            }
        }
    }
}

void CSItemOption::RenderOptionHelper(void)
{
    if (m_byRenderOptionList <= 0) return;

    int TextNum = 0;
    int sx = 0, sy = 0;
    ZeroMemory(TextListColor, sizeof TextListColor);
    for (int i = 0; i < 30; i++)
    {
        TextList[i][0] = NULL;
    }

    ITEM_SET_OPTION& setOption = m_ItemSetOption[m_byRenderOptionList - 1];
    if (setOption.byOptionCount >= 255)
    {
        m_byRenderOptionList = 0;
        return;
    }

    BYTE    option1 = 255;
    BYTE    option2 = 255;
    BYTE    value1 = 255;
    BYTE    value2 = 255;
    swprintf(TextList[TextNum], L"\n"); TextNum++;
    swprintf(TextList[TextNum], L"%s %s %s", setOption.strSetName, GlobalText[1089], GlobalText[159]);
    TextListColor[TextNum] = TEXT_COLOR_YELLOW;
    TextNum++;

    swprintf(TextList[TextNum], L"\n"); TextNum++;
    swprintf(TextList[TextNum], L"\n"); TextNum++;

    for (int i = 0; i < 13; ++i)
    {
        if (i < 6)
        {
            option1 = setOption.byStandardOption[i][0];
            option2 = setOption.byStandardOption[i][1];
            value1 = setOption.byStandardOptionValue[i][0];
            value2 = setOption.byStandardOptionValue[i][1];
        }
        else if (i < 8)
        {
            option1 = setOption.byExtOption[i - 6];
            value1 = setOption.byExtOptionValue[i - 6];
        }
        else
        {
            option1 = setOption.byFullOption[i - 8];
            value1 = setOption.byFullOptionValue[i - 8];
        }
        if (option1 != 255)
        {
            getExplainText(TextList[TextNum], option1, value1, 0);
            TextListColor[TextNum] = TEXT_COLOR_BLUE;
            TextBold[TextNum] = false; TextNum++;
        }
        if (option2 != 255)
        {
            getExplainText(TextList[TextNum], option2, value2, 0);
            TextListColor[TextNum] = TEXT_COLOR_BLUE;
            TextBold[TextNum] = false; TextNum++;
        }
    }

    swprintf(TextList[TextNum], L"\n"); TextNum++;
    swprintf(TextList[TextNum], L"\n"); TextNum++;

    SIZE TextSize = { 0, 0 };
    GetTextExtentPoint32(g_pRenderText->GetFontDC(), TextList[0], 1, &TextSize);
    RenderTipTextList(sx, sy, TextNum, 0);
}

int CSItemOption::GetSetItmeCount(const ITEM* pselecteditem)
{
    ITEM_SET_TYPE& itemsettype = m_ItemSetType[pselecteditem->Type];
    BYTE subtype = itemsettype.byOption[(pselecteditem->ExtOption % 0x04) - 1];

    int setitemcount = 0;

    for (int j = 0; j < MAX_ITEM; j++)
    {
        ITEM_SET_TYPE& temptype = m_ItemSetType[j];
        for (int i = 0; i < 2; i++)
        {
            BYTE tempsubtype = temptype.byOption[i];

            if (subtype == tempsubtype)
            {
                setitemcount++;
            }
        }
    }

    return setitemcount;
}

bool CSItemOption::isFullseteffect(const ITEM* pselecteditem)
{
    int mysetitemcount = 0;

    ITEM_SET_TYPE& selectedItemType = m_ItemSetType[pselecteditem->Type];
    BYTE selectedItemOption = selectedItemType.byOption[(pselecteditem->ExtOption % 0x04) - 1];
    ITEM_SET_OPTION& selecteditemoption = m_ItemSetOption[selectedItemOption];
    int	Cmp_Buff[10] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };

    for (int i = 0; i < MAX_EQUIPMENT; i++)
    {
        ITEM* p = &CharacterMachine->Equipment[i];

        if (p)
        {
            bool Continue_Set = false;
            for (int ipjh = 0; ipjh < mysetitemcount; ipjh++)
            {
                if (p->Type == Cmp_Buff[ipjh])
                {
                    Continue_Set = true;
                    break;
                }
            }
            if (Continue_Set == true)
                continue;

            ITEM_SET_TYPE& myitemSType = m_ItemSetType[p->Type];
            BYTE myItemOption = myitemSType.byOption[(p->ExtOption % 0x04) - 1];
            ITEM_SET_OPTION& setOption = m_ItemSetOption[myItemOption];

            if (wcscmp(selecteditemoption.strSetName, setOption.strSetName) == NULL)
            {
                Cmp_Buff[mysetitemcount] = p->Type;
                mysetitemcount++;
            }
        }
    }

    if (mysetitemcount == GetSetItmeCount(pselecteditem))
        return true;
    else
        return false;
}

int     CSItemOption::RenderSetOptionListInItem(const ITEM* ip, int TextNum, bool bIsEquippedItem)
{
    ITEM_SET_TYPE& itemSType = m_ItemSetType[ip->Type];

    m_bySelectedItemOption = itemSType.byOption[(ip->ExtOption % 0x04) - 1];

    if (m_bySelectedItemOption <= 0 || m_bySelectedItemOption == 255) return TextNum;

    int TNum = TextNum;

    ITEM_SET_OPTION& setOption = m_ItemSetOption[m_bySelectedItemOption];
    if (setOption.byOptionCount >= 255)
    {
        m_bySelectedItemOption = 0;
        return TNum;
    }

    BYTE    option1 = 255;
    BYTE    option2 = 255;
    BYTE    value1 = 255;
    BYTE    value2 = 255;
    BYTE    count1 = 0;

    BYTE    byLimitOptionNum;

    if (m_bySetOptionANum > 0)
        byLimitOptionNum = m_bySetOptionANum - m_bySameSetItem;//m_bySetOptionANum-1;
    else
        byLimitOptionNum = 0;

    if (m_bySetOptionBNum > 0)
        byLimitOptionNum += m_bySetOptionBNum - m_bySameSetItem;//m_bySetOptionANum-1;

    count1 = Search_From_EquippedSetItemNameSequence(setOption.strSetName);

    if (255 == count1)
    {
        byLimitOptionNum = 0;
    }
    else
    {
        byLimitOptionNum = abs((m_bySetOptionANum + m_bySetOptionBNum) - m_bySameSetItem);
    }

    swprintf(TextList[TNum], L"\n"); TNum += 1;
    swprintf(TextList[TNum], L"%s %s", GlobalText[1089], GlobalText[159]);
    TextListColor[TNum] = TEXT_COLOR_YELLOW;
    TNum++;

    swprintf(TextList[TNum], L"\n"); TNum++;
    swprintf(TextList[TNum], L"\n"); TNum++;

    bool isfulloption = isFullseteffect(ip);

    if (isfulloption)
    {
        byLimitOptionNum = 13;
    }

    BYTE byCurrentSelectedSlotIndex = ip->bySelectedSlotIndex;

    int iLimitOptionCount = m_arLimitSetItemOptionCount[byCurrentSelectedSlotIndex] - 1;

    for (int i = 0; i <= MAX_SETITEM_OPTIONS; ++i)
    {
        if (i < 6)
        {
            option1 = setOption.byStandardOption[i][0];
            option2 = setOption.byStandardOption[i][1];
            value1 = setOption.byStandardOptionValue[i][0];
            value2 = setOption.byStandardOptionValue[i][1];
        }
        else if (i < 8)
        {
            if (((ip->ExtOption % 0x04) - 1) == 0)
            {
                option1 = setOption.byExtOption[i - 6];
                value1 = setOption.byExtOptionValue[i - 6];
            }
            else
            {
                option2 = setOption.byExtOption[i - 6];
                value2 = setOption.byExtOptionValue[i - 6];
            }
        }
        else
        {
            if (((ip->ExtOption % 0x04) - 1) == 0)
            {
                option1 = setOption.byFullOption[i - 8];
                value1 = setOption.byFullOptionValue[i - 8];
            }
            else
            {
                option2 = setOption.byFullOption[i - 8];
                value2 = setOption.byFullOptionValue[i - 8];
            }

            if (isfulloption)
            {
                byLimitOptionNum = 13;
            }
            else
            {
                byLimitOptionNum = 255;
            }
        }

        if (option1 != 255)
        {
            getExplainText(TextList[TNum], option1, value1, 0);

            if (m_bySetOptionList[count1][0] == option1
                && byLimitOptionNum != 255
                && iLimitOptionCount > i
                && byLimitOptionNum != 0
                && bIsEquippedItem == true
                )
            {
                TextListColor[TNum] = TEXT_COLOR_BLUE;
                count1++;
            }
            else
            {
                TextListColor[TNum] = TEXT_COLOR_GRAY;
            }
            TextBold[TNum] = false; TNum++;
        }
        if (option2 != 255)
        {
            getExplainText(TextList[TNum], option2, value2, 0);
            if (m_bySetOptionList[count1][0] == option2
                && byLimitOptionNum != 255
                && iLimitOptionCount > i
                && byLimitOptionNum != 0
                )
            {
                TextListColor[TNum] = TEXT_COLOR_BLUE;
                count1++;
            }
            else
            {
                TextListColor[TNum] = TEXT_COLOR_GRAY;
            }
            TextBold[TNum] = false; TNum++;
        }
    }
    swprintf(TextList[TNum], L"\n"); TNum++;
    swprintf(TextList[TNum], L"\n"); TNum++;

    return TNum;
}

BYTE CSItemOption::GetSetOptionANum()
{
    return m_bySetOptionANum;
}

BYTE CSItemOption::GetSetOptionBNum()
{
    return m_bySetOptionBNum;
}

void CSItemOption::SetViewOptionList(bool bView)
{
    m_bViewOptionList = bView;
}

bool CSItemOption::IsViewOptionList()
{
    return m_bViewOptionList;
}