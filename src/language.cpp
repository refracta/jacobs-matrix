/*
 * Runtime language selection for the web port.
 */

#include "language.h"

#include <fstream>
#include <string.h>
using namespace std;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define JACOB_LANGUAGE_PATH "/save/jacob.lang"
#else
#define JACOB_LANGUAGE_PATH "jacob.lang"
#endif

struct LANGUAGE_ENTRY
{
    const char *english;
    const char *korean;
};

static LANGUAGE_NAMES glbLanguage = LANGUAGE_ENGLISH;

static const LANGUAGE_ENTRY glbLanguageEntries[] =
{
    { "Instructions", "도움말" },
    { "Play", "게임 시작" },
    { "Language", "언어" },
    { "Volume", "음량" },
    { "Flame Quality", "불꽃 품질" },
    { "Current Role", "현재 역할" },
    { "Toggle Full Screen", "전체 화면 전환" },
    { "Quit", "종료" },

    { "English", "English" },
    { "Korean", "한국어" },
    { "No Flames", "불꽃 없음" },
    { "Low", "낮음" },
    { "Middle", "보통" },
    { "High", "높음" },
    { "Silly", "최고" },

    { "Training Grounds", "훈련장" },
    { "Easy", "쉬움" },
    { "Straight Forward", "직진" },
    { "Hard", "어려움" },
    { "Harder", "더 어려움" },
    { "Difficult", "매우 어려움" },
    { "Impossible", "불가능" },
    { "Implausible", "거의 불가능" },
    { "Out of Adjectives", "형용사 고갈" },
    { "Final Level", "최종 레벨" },

    { "No Weapon.\n", "무기 없음.\n" },
    { "No Spellbook.", "마법책 없음." },
    { "Press 'v' for Victory!", "승리하려면 'v'!" },
    { " Dead, awaiting revival... ", " 사망, 부활 대기 중... " },
    { " Jacobian:\n", " 야코비안:\n" },

    { "Your time: ", "기록: " },
    { "This is now the time to beat for this level.\n", "이 기록이 이 레벨의 새로운 기준 기록입니다.\n" },
    { "You did not beat the previous best of ", "이전 최고 기록 " },
    { ", you fell short by ", "보다 " },
    { "You beat the previous best of ", "이전 최고 기록 " },
    { " by ", "을 " },
    { "Dead people can't move.  ", "죽은 상태에서는 움직일 수 없습니다.  " },
    { "Your time has run out!  ", "시간이 다 되었습니다!  " },
    { "The wall proved too unstable to hold a portal.", "벽이 너무 불안정해서 차원문을 유지할 수 없습니다." },

    { "Select a new role for yourself.  ", "새 역할을 선택하세요.  " },
    { "Cancelled.  ", "취소했습니다.  " },
    { "Cast spell in what direction?  ", "어느 방향으로 주문을 시전할까요?  " },
    { "The dead cannot declare victory.  ", "죽은 상태에서는 승리를 선언할 수 없습니다.  " },
    { "Reach the gold room first!  ", "먼저 황금 방에 도달하세요!  " },

    { "Weapon:", "무기:" },
    { "Spellbook:", "마법책:" },
    { "Power", "위력" },
    { "Accuracy", "명중률" },
    { "Consistency", "안정성" },
    { "Range", "사거리" },
    { "Area", "범위" },
    { "Low-Mana", "저마나" },

    { "barbarian", "야만인" },
    { "thief", "도적" },
    { "fighter", "전사" },
    { "paladin", "성기사" },
    { "tourist", "관광객" },
    { "monk", "수도승" },
    { "priest", "사제" },
    { "shaman", "주술사" },
    { "wizard", "마법사" },

    // Dynamic message subjects and objects.
    { "you", "당신" },
    { "You", "당신" },
    { "ant", "개미" },
    { "bat", "박쥐" },
    { "cat", "고양이" },
    { "dog", "개" },
    { "echidna", "바늘두더지" },
    { "frog", "개구리" },
    { "ghost", "유령" },
    { "horse", "말" },
    { "imp", "임프" },
    { "jack rabbit", "산토끼" },
    { "kobold", "코볼트" },
    { "lichen", "지의류" },
    { "mouse", "생쥐" },
    { "newt", "영원" },
    { "orc", "오크" },
    { "penguin", "펭귄" },
    { "queen bee", "여왕벌" },
    { "rat", "쥐" },
    { "snake", "뱀" },
    { "turtle", "거북이" },
    { "unicorn", "유니콘" },
    { "vampire", "흡혈귀" },
    { "warthog", "혹멧돼지" },
    { "grid bug", "그리드 벌레" },
    { "yellow light", "노란 빛" },
    { "zombie", "좀비" },
    { "ape", "유인원" },
    { "polar bear", "북극곰" },
    { "centaur", "켄타우로스" },
    { "dragon", "용" },
    { "elephant", "코끼리" },
    { "fungus", "균류" },
    { "giant", "거인" },
    { "hippopotamus", "하마" },
    { "icebeast", "얼음 야수" },
    { "jaguar", "재규어" },
    { "kleptomaniac", "도벽 환자" },
    { "lich", "리치" },
    { "mummy", "미라" },
    { "naga", "나가" },
    { "ogre", "오우거" },
    { "python", "비단뱀" },
    { "quoll", "주머니고양이" },
    { "rous", "거대 설치류" },
    { "purple slug", "보라색 민달팽이" },
    { "troll", "트롤" },
    { "umber hulk", "엄버 헐크" },
    { "vampire lord", "흡혈귀 군주" },
    { "wraith", "망령" },
    { "xorn", "조른" },
    { "yeti", "예티" },
    { "skeleton captain", "해골 대장" },
    { "evil demon boss", "사악한 악마 우두머리" },

    { "speed boots", "신속의 장화" },
    { "quick boost potion", "가속 물약" },
    { "Blinded", "실명" },
    { "Slow", "둔화" },
    { "ring of regeneration", "재생의 반지" },
    { "cloak of invulnerability", "무적의 망토" },
    { "healing potion", "치유 물약" },
    { "mana potion", "마나 물약" },
    { "nothing", "아무것도" },
    { "bare hands", "맨손" },
    { "pincers", "집게발" },
    { "teeth", "이빨" },
    { "claws", "발톱" },
    { "mouth", "입" },
    { "spectral cold", "영체의 냉기" },
    { "hooves", "발굽" },
    { "hands", "손" },
    { "fireball", "화염구" },
    { "rocks", "돌멩이" },
    { "none", "없음" },
    { "beak", "부리" },
    { "stinger", "독침" },
    { "horn", "뿔" },
    { "tusks", "엄니" },
    { "electric field", "전기장" },
    { "bright light", "강한 빛" },
    { "sword", "검" },
    { "bow and arrow", "활과 화살" },
    { "firebreath", "화염 숨결" },
    { "feet", "발" },
    { "giant club", "거대 곤봉" },
    { "boulder", "바위" },
    { "icey grasp", "얼어붙은 손아귀" },
    { "fists", "주먹" },
    { "cast death bolt", "죽음의 화살" },
    { "spears", "창" },
    { "spiked club", "가시 곤봉" },
    { "swallows whole", "통째로 삼키기" },
    { "acidic slime", "산성 점액" },
    { "long bow", "장궁" },
    { "flaming sword", "화염 검" },

    { "wall", "벽" },
    { "floor", "바닥" },
    { "downstairs", "아래층 계단" },
    { "upstairs", "위층 계단" },
    { "snow covered ground", "눈 덮인 땅" },
    { "path", "길" },
    { "grass", "풀밭" },
    { "field", "들판" },
    { "fire", "불" },
    { "portal", "차원문" },
    { "orange portal", "주황색 차원문" },
    { "blue portal", "파란색 차원문" },
    { "broken wall", "부서진 벽" },
    { "door", "문" },
    { "mountain", "산" },
    { "icy mountain", "얼어붙은 산" },
    { "snowy pass", "눈 덮인 고갯길" },
    { "bridge", "다리" },
    { "water", "물" },
    { "forest", "숲" },

    { "Right", "오른쪽" },
    { "Down-Right", "오른쪽 아래" },
    { "Down", "아래" },
    { "Down-Left", "왼쪽 아래" },
    { "Left", "왼쪽" },
    { "Up-Left", "왼쪽 위" },
    { "Up", "위" },
    { "Up-Right", "오른쪽 위" },

    // Procedurally assembled equipment names.
    { "broken", "망가진" },
    { "worn", "낡은" },
    { "chipped", "이가 빠진" },
    { "scratched", "긁힌" },
    { "plain", "평범한" },
    { "sharp", "날카로운" },
    { "homing", "유도" },
    { "glowing", "빛나는" },
    { "runed", "룬 문자" },
    { "artifact", "유물급" },
    { "wooden", "나무" },
    { "stone", "돌" },
    { "bronze", "청동" },
    { "iron", "철" },
    { "steel", "강철" },
    { "silver", "은" },
    { "gold", "금" },
    { "adamantium", "아다만티움" },
    { "unobtanium", "언옵테이늄" },
    { "diamond", "다이아몬드" },
    { "emerald", "에메랄드" },
    { "quartz", "석영" },
    { "ruby", "루비" },
    { "zircon", "지르콘" },
    { "knife", "단검" },
    { "short sword", "단도" },
    { "long sword", "장검" },
    { "axe", "도끼" },
    { "battle-axe", "전투 도끼" },
    { "club", "곤봉" },
    { "mace", "철퇴" },
    { "hammer", "망치" },
    { "warhammer", "전투 망치" },
    { "dagger", "비수" },
    { "short spear", "단창" },
    { "spear", "창" },
    { "paper", "종이" },
    { "papyrus", "파피루스" },
    { "leather", "가죽" },
    { "metal", "금속" },
    { "human skin", "인피" },
    { "dragon hide", "용가죽" },
    { "naugahyde", "인조가죽" },
    { "shock", "충격" },
    { "lightning", "번개" },
    { "thunder", "천둥" },
    { "chill", "냉기" },
    { "cold", "한기" },
    { "frost", "서리" },
    { "poison", "독" },
    { "force", "역장" },
    { "bolt", "화살" },
    { "ball", "구체" },
    { "storm", "폭풍" },

    // Past-tense verb phrases used by combat messages.
    { "punch", "주먹으로 쳤다" },
    { "pinch", "집게로 꼬집었다" },
    { "bite", "물었다" },
    { "slash", "베었다" },
    { "touch", "건드렸다" },
    { "kick", "걷어찼다" },
    { "burn", "불태웠다" },
    { "stab", "찔렀다" },
    { "pierce", "꿰뚫었다" },
    { "brush", "스쳤다" },
    { "nibble", "갉았다" },
    { "bash", "후려쳤다" },
    { "sting", "쏘았다" },
    { "butt", "들이받았다" },
    { "drain", "생명력을 빨아들였다" },
    { "zap", "감전시켰다" },
    { "blind", "눈을 멀게 했다" },
    { "claw", "할퀴었다" },
    { "trample", "짓밟았다" },
    { "crush", "으스러뜨렸다" },
    { "freeze", "얼렸다" },
    { "frisk", "뒤졌다" },
    { "swallow", "삼켰다" },
    { "slime", "점액을 묻혔다" },
    { "eviscerate", "난도질했다" },
    { "rend", "찢어발겼다" },
    { "hit", "공격했다" },
    { "cleave", "내리갈랐다" },
    { "jab", "찌르듯 공격했다" },
    { "impale", "꿰뚫었다" },
};

static void
language_update_web_page()
{
#ifdef __EMSCRIPTEN__
    MAIN_THREAD_EM_ASM({
	if (globalThis.__jacobSetLanguage) globalThis.__jacobSetLanguage(!!$0);
    }, glbLanguage == LANGUAGE_KOREAN);
#endif
}

static void
language_store()
{
    ofstream os(JACOB_LANGUAGE_PATH);
    if (!os)
	return;
    os << (glbLanguage == LANGUAGE_KOREAN ? "ko" : "en");
    os.close();

#ifdef __EMSCRIPTEN__
    MAIN_THREAD_EM_ASM({
	try {
	    localStorage.setItem("jacobs-matrix-language", $0 ? "ko" : "en");
	} catch (error) {
	    console.warn("Could not persist the language preference", error);
	}
    }, glbLanguage == LANGUAGE_KOREAN);

    EM_ASM({
	if (typeof FS !== "undefined" && FS.syncfs) {
	    FS.syncfs(false, function(error) {
		if (error) console.warn("Could not persist the language setting", error);
	    });
	}
    });
#endif
}

void
language_init()
{
    ifstream is(JACOB_LANGUAGE_PATH);
    char code[8];
    code[0] = '\0';
    if (is)
	is >> code;

#ifdef __EMSCRIPTEN__
    int browser_language = MAIN_THREAD_EM_ASM_INT({
	try {
	    var preference = localStorage.getItem("jacobs-matrix-language");
	    if (preference === "ko") return 1;
	    if (preference === "en") return 0;
	    return -1;
	} catch (error) {
	    return -1;
	}
    });
#else
    int browser_language = -1;
#endif


    if (browser_language >= 0)
	glbLanguage = browser_language ? LANGUAGE_KOREAN : LANGUAGE_ENGLISH;
    else if (!strcmp(code, "ko"))
	glbLanguage = LANGUAGE_KOREAN;
    else
	glbLanguage = LANGUAGE_ENGLISH;

    language_update_web_page();
}

void
language_set(LANGUAGE_NAMES language)
{
    if (language < LANGUAGE_ENGLISH || language >= NUM_LANGUAGES)
	language = LANGUAGE_ENGLISH;

    glbLanguage = language;
    language_update_web_page();
    language_store();
}

LANGUAGE_NAMES
language_get()
{
    return glbLanguage;
}

bool
language_is_korean()
{
    return glbLanguage == LANGUAGE_KOREAN;
}

const char *
language_text(const char *english)
{
    unsigned int i;

    if (!english || !language_is_korean())
	return english;

    for (i = 0; i < sizeof(glbLanguageEntries) / sizeof(glbLanguageEntries[0]); i++)
    {
	if (!strcmp(english, glbLanguageEntries[i].english))
	    return glbLanguageEntries[i].korean;
    }

    return english;
}
