/*
 * wordle.cpp
 *
 * Wordle minigame for TermiCraft.
 *
 * The player has 5 guesses to identify a hidden word.
 * Letters are colored green (correct position), yellow (wrong position),
 * or gray (not in word). Difficulty sets word length: 4, 5, or 6 letters.
 *
 * Author: Aryan
 */

#include "colors.h"
#include "menu.h"
#include "types.h"
#include <cctype>
#include <iostream>
#include <unistd.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>

static void showWordleEndScreen(bool won, const std::string& target) {
    clearAndCenterV(won ? 12 : 11);

    std::string Aw = hpad(63);
    std::string Ag = hpad(80);

    if (won) {
        std::cout << COLOR_BOLD_GREEN << "\n"
            << Aw << "      ██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗██╗███╗   ██╗\n"
            << Aw << "      ╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██║████╗  ██║\n"
            << Aw << "       ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║██╔██╗ ██║\n"
            << Aw << "        ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║██║╚██╗██║\n"
            << Aw << "         ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝██║██║ ╚████║\n"
            << Aw << "         ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝\n"
            << COLOR_RESET << "\n";

        std::cout << "\n" << hpad(31) << "Correct! You guessed the word.\n";
    } else {
        std::string wordStr = "The word was: ";
        for (char c : target) wordStr += (char)toupper(c);

        std::cout << COLOR_BOLD_RED << "\n"
            << Ag << "   ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗\n"
            << Ag << "  ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗\n"
            << Ag << "  ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝\n"
            << Ag << "  ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗\n"
            << Ag << "  ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║\n"
            << Ag << "   ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝\n"
            << COLOR_RESET << "\n";

        std::cout << "\n" << hpad((int)wordStr.size()) << wordStr << "\n";
    }
}


// ─── WORD LISTS ───────────────────────────────────────────────────────────────

static const std::vector<std::string> WORDS_4 = {
    
    "able","acid",
"aged","also",
"area","army",
"away","back",
"ball","band",
"bank","base",
"bath","bear",
"beat","bell",
"best","bill",
"bird","blow",
"blue","boat",
"body","bomb",
"bond","bone",
"book","boom",
"born","burn",
"busy","call",
"calm","card",
"care","case",
"cash","cave",
"chip","city",
"clam","clan",
"claw","clay",
"clip","club",
"clue","coal",
"coat","code",
"coil","coin",
"cold","come",
"cook","cool",
"copy","cord",
"core","corn",
"cost","crab",
"crew","crop",
"crow","cube",
"cure","curl",
"cute","damp",
"dare","dark",
"dart","dash",
"data","date",
"dawn","dead",
"deaf","deal",
"dear","debt",
"deck","deed",
"deep","deny",
"desk","dice",
"diet","dirt",
"disk","dock",
"done","door",
"dose","down",
"drag","draw",
"drip","drop",
"drug","drum",
"duck","duel",
"dull","dump",
"dune","dusk",
"dust","duty",
"earn","ease",
"east","easy",
"edge","emit",
"epic","even",
"ever","evil",
"face","fact",
"fail","fair",
"fake","fall",
"fame","farm",
"fast","fate",
"fear","feat",
"feed","feel",
"feet","fell",
"felt","file",
"fill","film",
"find","fine",
"fire","firm",
"fish","fist",
"flag","flat",
"flaw","flew",
"flip","flow",
"foam","foil",
"fold","folk",
"fond","font",
"food","fool",
"foot","ford",
"fork","form",
"fort","foul",
"four","free",
"fuel","full",
"fund","fuse",
"gain","game",
"gave","gaze",
"gear","gift",
"give","glad",
"glow","glue",
"goal","gold",
"gone","good",
"grab","gray",
"grew","grid",
"grim","grin",
"grip","grow",
"gulf","hack",
"hail","half",
"hall","hand",
"hang","hard",
"harm","hate",
"have","head",
"heal","heap",
"hear","heat",
"held","help",
"herd","hero",
"hide","high",
"hill","hint",
"hire","hold",
"hole","holy",
"home","hope",
"host","hour",
"huge","hunt",
"hurt","idea",
"inch","into",
"iron","item",
"jack","jail",
"join","joke",
"jump","jury",
"just","keen",
"keep","kent",
"kept","kick",
"kill","kind",
"king","knee",
"knew","know",
"lack","lady",
"laid","lake",
"land","lane",
"last","late",
"lead","leaf",
"lean","left",
"lend","lens",
"less","lest",
"life","lift",
"like","line",
"link","list",
"live","load",
"loan","lock",
"logo","long",
"look","lord",
"lose","loss",
"lost","loud",
"love","luck",
"made","mail",
"main","make",
"male","many",
"mark","mass",
"mate","meal",
"mean","meat",
"meet","menu",
"mere","mild",
"mile","milk",
"mind","mine",
"miss","mode",
"mood","moon",
"more","most",
"move","much",
"must","name",
"navy","near",
"neck","need",
"news","next",
"nice","nick",
"nine","none",
"nose","note",
"okay","once",
"only","onto",
"open","oral",
"over","pace",
"pack","page",
"paid","pain",
"pair","pale",
"palm","park",
"part","pass",
"past","path",
"peak","pick",
"pink","pipe",
"plan","play",
"plot","plug",
"plus","poll",
"pool","poor",
"port","post",
"pull","pure",
"push","race",
"rail","rain",
"rank","rare",
"rate","read",
"real","rear",
"rely","rent",
"rest","rice",
"rich","ride",
"ring","rise",
"risk","road",
"rock","role",
"roll","roof",
"room","root",
"rope","rose",
"rule","rush",
"safe","said",
"sake","sale",
"salt","same",
"sand","save",
"seat","seed",
"seek","seem",
"seen","self",
"sell","send",
"sent","ship",
"shop","shot",
"show","shut",
"sick","side",
"sign","site",
"size","skin",
"slip","slow",
"snow","soft",
"soil","sold",
"sole","some",
"song","soon",
"sort","soul",
"spot","star",
"stay","step",
"stop","such",
"suit","sure",
"take","tale",
"talk","tall",
"tank","tape",
"task","team",
"tech","tell",
"tend","term",
"test","text",
"than","that",
"them","then",
"they","thin",
"this","thus",
"tide","till",
"time","tiny",
"told","tone",
"took","tool",
"tour","town",
"tree","trip",
"true","tune",
"turn","twin",
"type","unit",
"upon","used",
"user","vary",
"vast","very",
"vice","view",
"vote","wage",
"wait","wake",
"walk","wall",
"want","ward",
"warm","wash",
"wave","ways",
"weak","wear",
"week","well",
"went","were",
"west","what",
"when","whom",
"wide","wife",
"wild","will",
"wind","wine",
"wing","wire",
"wise","wish",
"with","wood",
"word","wore",
"work","yard",
"yeah","year",
"your","zero",
"zone"



};

static const std::vector<std::string> WORDS_5 = {



"adore","adorn",
"afire","aisle",
"alone","amaze",
"amber","amend",
"angel","anger",
"angle","ankle",
"apple","april",
"argue","arise",
"armed","arrow",
"asset","audio",
"audit","awful",
"badge","baker",
"balmy","basil",
"beach","beard",
"beast","begun",
"belly","bench",
"berry","blame",
"bland","blast",
"blaze","bleak",
"blend","bless",
"blimp","blink",
"bliss","block",
"bloom","bluff",
"blunt","blush",
"board","boast",
"bonus","booth",
"bound","brace",
"brain","brave",
"break","brick",
"bride","brief",
"bring","broad",
"broil","brown",
"brush","build",
"built","bulky",
"bunch","burst",
"cabin","cable",
"candy","cargo",
"carry","catch",
"chain","chair",
"chalk","charm",
"chase","cheer",
"chess","chief",
"child","chill",
"chime","chunk",
"civil","claim",
"class","clean",
"clear","climb",
"cloak","clock",
"close","cloud",
"clown","coast",
"comet","crack",
"craft","crane",
"crash","crazy",
"cream","creek",
"creep","crime",
"crisp","cross",
"crowd","crown",
"cruel","crush",
"curve","cycle",
"daily","dance",
"dealt","delay",
"delta","demon",
"depth","diner",
"dirty","doubt",
"dozen","draft",
"drain","dream",
"dress","drift",
"drink","drive",
"eager","early",
"earth","eight",
"empty","enjoy",
"enter","equal",
"error","event",
"every","exact",
"exile","extra",
"fable","faith",
"fancy","feast",
"field","fiery",
"final","flame",
"flash","fleet",
"flood","focus",
"force","forge",
"frame","fresh",
"fruit","giant",
"glide","grace",
"grade","grain",
"grand","grant",
"grasp","great",
"green","greet",
"grind","group",
"guard","guess",
"guest","guide",
"habit","happy",
"harsh","heart",
"heavy","honey",
"honor","horse",
"house","human",
"humor","ideal",
"image","index",
"inner","jolly",
"judge","knock",
"known","large",
"laser","laugh",
"learn","lemon",
"light","limit",
"lucky","magic",
"major","march",
"match","metal",
"minor","model",
"money","mount",
"music","noble",
"noise","north",
"novel","ocean",
"offer","order",
"paint","panel",
"party","peace",
"phone","photo",
"piano","piece",
"pilot","place",
"plant","plate",
"point","power",
"pride","prime",
"print","prize",
"queen","quick",
"quiet","radio",
"reach","react",
"ready","river",
"robot","rough",
"round","scene",
"scope","score",
"shape","share",
"shift","shine",
"shore","skill",
"sleep","smile",
"smoke","sound",
"space","speak",
"spend","sport",
"stage","start",
"steel","stick",
"stone","store",
"storm","story",
"study","sugar",
"table","taste",
"teach","theme",
"think","throw",
"tight","trade",
"train","treat",
"trial","truck",
"truth","twist",
"under","unite",
"value","voice",
"water","wheel",
"white","whole",
"woman","world",
"worry","write",
"young","youth",
"zebra"



};

static const std::vector<std::string> WORDS_6 = {



"abroad","absorb",
"accent","accept",
"access","action",
"advent","aerial",
"affect","afford",
"agency","agenda",
"alight","allege",
"allied","almost",
"alpine","ambush",
"amount","anchor",
"animal","annual",
"answer","anthem",
"anyway","appear",
"arctic","ardent",
"armory","around",
"arrest","arrive",
"ascend","aspect",
"assail","assign",
"assist","assume",
"assure","attest",
"attire","august",
"author","badger",
"banter","barren",
"battle","beacon",
"beetle","before",
"behind","belief",
"belong","better",
"bitten","bitter",
"blazer","bleach",
"blouse","border",
"borrow","bottle",
"bounce","branch",
"breeze","bridge",
"bright","broken",
"bucket","buckle",
"burden","burrow",
"butter","button",
"cactus","candle",
"canopy","castle",
"casual","cavern",
"cement","center",
"cereal","chance",
"change","charge",
"choose","chosen",
"chrome","cipher",
"circle","citrus",
"clover","cobalt",
"cobble","coerce",
"coffee","column",
"combat","coming",
"commit","common",
"compel","comply",
"concur","condor",
"cosmos","cotton",
"couple","course",
"cousin","covert",
"create","credit",
"cringe","crispy",
"crouch","cruise",
"dagger","damage",
"danger","dangle",
"darken","debris",
"decide","defend",
"degree","demand",
"denial","depart",
"depend","deploy",
"desert","design",
"desire","devote",
"differ","dilute",
"direct","divide",
"donkey","double",
"dragon","drawer",
"driven","easily",
"effect","effort",
"either","eldest",
"embark","empire",
"enable","encode",
"endure","engage",
"ensure","entire",
"entity","enzyme",
"escape","evolve",
"excite","exotic",
"expand","expect",
"expire","export",
"expose","extend",
"famine","fathom",
"feline","filter",
"finger","finite",
"flight","flinch",
"flower","follow",
"forbid","forest",
"formal","foster",
"frozen","fumble",
"gamble","garden",
"garlic","gentle",
"giggle","ginger",
"global","goblet",
"gospel","govern",
"guitar","habits",
"hammer","handle",
"happen","harbor",
"hazard","health",
"heaven","helmet",
"honest","honour",
"horror","hunter",
"impact","import",
"income","indeed",
"indoor","injury",
"insist","insult",
"intend","invent",
"invest","island",
"jacket","jungle",
"kidney","kitten",
"latter","lawyer",
"lender","lesson",
"letter","liquid",
"lizard","lounge",
"lumber","magnet",
"manage","marble",
"market","master",
"matter","memory",
"method","middle",
"mirror","moment",
"motion","museum",
"mutual","nation",
"nature","needle",
"normal","notion",
"number","object",
"office","orange",
"origin","output",
"oxygen","packet",
"palace","parade",
"parent","pastor",
"patrol","peanut",
"pepper","person",
"pillar","planet",
"poetry","police",
"policy","praise",
"prayer","preach",
"profit","proper",
"public","puzzle",
"quartz","rabbit",
"random","reason",
"refuse","regard",
"region","reject",
"relief","remind",
"repair","repeat",
"report","rescue",
"resist","result",
"retire","reveal",
"revise","reward",
"ribbon","rocket",
"rubber","saddle",
"salary","sandal",
"satisfy","scarce",
"school","scream",
"search","season",
"secret","secure",
"segment","select",
"senior","series",
"shadow","shiver",
"signal","silence",
"simple","sister",
"slogan","soccer",
"social","solemn",
"source","speech",
"spirit","stable",
"strain","string",
"subtle","success",
"sudden","summit",
"supply","system",
"talent","target",
"temple","theory",
"thread","ticket",
"timber","tissue",
"tobacco","tongue",
"travel","tunnel",
"twenty","typical",
"update","useful",
"valley","vanish",
"vector","victim",
"vision","volume",
"wander","wealth",
"weapon","winter",
"wisdom","within",
"wonder","worker",
"writer","yellow",
"zephyr"


};

static const int MAX_GUESSES = 5;

// ─── HELPERS ──────────────────────────────────────────────────────────────────

static std::string toLowerString(std::string s) {
    for (char& c : s) c = tolower(c);
    return s;
}

static void printWordleTitle(int wordLength) {
    const int BW = 40;
    std::string P = hpad(BW + 2);

    auto centerLine = [&](const std::string& text, int displayWidth = -1) {
    int width = (displayWidth >= 0) ? displayWidth : (int)text.size();
    int pad = BW - width;

    int left = pad / 2;
    int right = pad - left;
        std::cout << P << "\xe2\x95\x91" << std::string(left, ' ') << text
                  << std::string(right, ' ') << "\xe2\x95\x91\n";
    };

    std::cout << COLOR_BOLD_CYAN << "\n";
    std::cout << P << "\xe2\x95\x94";
    for (int i = 0; i < BW; i++) std::cout << "\xe2\x95\x90";
    std::cout << "\xe2\x95\x97\n";
    centerLine("⭐ WORDLE MINIGAME ⭐", 21);
    centerLine("Guess the " + std::to_string(wordLength) + "-letter word!");
    centerLine("You have " + std::to_string(MAX_GUESSES) + " attempts.");
    std::cout << P << "\xe2\x95\x9a";
    for (int i = 0; i < BW; i++) std::cout << "\xe2\x95\x90";
    std::cout << "\xe2\x95\x9d\n";
    std::cout << COLOR_RESET << "\n";
}

/*
 * printGuessRow
 * Prints a colored tile row for one guess.
 * Input:  guess, target
 * Output: none
 */
static void printGuessRow(const std::string& guess, const std::string& target) {
    int len = target.size();
    std::vector<bool> used(len, false);
    std::vector<int> result(len, 0); // 0=gray, 1=yellow, 2=green

    for (int i = 0; i < len; i++) {
        if (guess[i] == target[i]) {
            result[i] = 2;
            used[i] = true;
        }
    }
    for (int i = 0; i < len; i++) {
        if (result[i] == 2) continue;
        for (int j = 0; j < len; j++) {
            if (!used[j] && guess[i] == target[j]) {
                result[i] = 1;
                used[j] = true;
                break;
            }
        }
    }

    // Each tile is 6 chars wide (  X  + space); center the row
    std::string P = hpad(len * 6);
    std::cout << P;
    for (int i = 0; i < len; i++) {
        if      (result[i] == 2) std::cout << BG_GREEN  << COLOR_BOLD_WHITE;
        else if (result[i] == 1) std::cout << BG_YELLOW << COLOR_BOLD_BLACK;
        else                     std::cout << "\033[48;5;240m" << COLOR_BOLD_WHITE;
        std::cout << "  " << (char)toupper(guess[i]) << "  " << COLOR_RESET << " ";
    }
    std::cout << "\n";
}

static void printEmptyRow(int wordLength) {
    std::string P = hpad(wordLength * 6);
    std::cout << P;
    for (int i = 0; i < wordLength; i++)
        std::cout << "\033[48;5;235m" << "     " << COLOR_RESET << " ";
    std::cout << "\n";
}

/*
 * printUsedLetters
 * Shows all 26 letters color-coded by best result seen so far.
 * Input:  guesses, target, wordLength
 * Output: none
 */
static void printUsedLetters(const std::vector<std::string>& guesses,
                              const std::string& target, int wordLength) {
    std::vector<int> status(26, 0); // 0=unused, 1=gray, 2=yellow, 3=green

    for (const std::string& guess : guesses) {
        std::vector<bool> used(wordLength, false);

        for (int i = 0; i < wordLength; i++) {
            if (guess[i] == target[i]) {
                status[guess[i] - 'a'] = 3;
                used[i] = true;
            }
        }
        for (int i = 0; i < (int)guess.size(); i++) {
            if (status[guess[i] - 'a'] == 3) continue;
            bool found = false;
            for (int j = 0; j < wordLength; j++) {
                if (!used[j] && guess[i] == target[j]) {
                    found = true;
                    used[j] = true;
                    break;
                }
            }
            if (found  && status[guess[i] - 'a'] < 2) status[guess[i] - 'a'] = 2;
            if (!found && status[guess[i] - 'a'] == 0) status[guess[i] - 'a'] = 1;
        }
    }

    std::string P = hpad(53);  // 26 letters × 2 chars + spacing ≈ 53
    std::cout << "\n" << P << "Letters used:\n" << P;
    for (int i = 0; i < 26; i++) {
        char c = 'A' + i;
        if      (status[i] == 3) std::cout << BG_GREEN        << COLOR_BOLD_WHITE << c << COLOR_RESET << " ";
        else if (status[i] == 2) std::cout << BG_YELLOW       << COLOR_BOLD_BLACK << c << COLOR_RESET << " ";
        else if (status[i] == 1) std::cout << "\033[48;5;240m" << COLOR_BOLD_WHITE << c << COLOR_RESET << " ";
        else                     std::cout << COLOR_DIM        << c << COLOR_RESET << " ";
        if (i == 12) std::cout << "\n" << P;
    }
    std::cout << "\n";
}

/*
 * getWordleGuess
 * Reads and validates a guess of exactly wordLength alpha characters.
 * Input:  wordLength
 * Output: validated lowercase guess string
 */
static std::string getWordleGuess(int wordLength) {
    std::string input;
    std::string P = hpad(50);
    while (true) {
        std::cout << COLOR_WHITE << "\n" << P << "Enter guess (or 'quit' to flee [-2x penalty]): " << COLOR_RESET;

        struct termios cooked;
        tcgetattr(STDIN_FILENO, &cooked);
        cooked.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

        std::getline(std::cin, input);

        struct termios raw = cooked;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        input = toLowerString(input);

        if (input == "quit" || input == "q") {
            g_minigameForfeited = true;
            return "__QUIT__";
        }
        if ((int)input.size() != wordLength) {
            std::cout << COLOR_RED << P << "Must be exactly " << wordLength << " letters.\n" << COLOR_RESET;
            continue;
        }
        bool allAlpha = true;
        for (char c : input) if (!isalpha(c)) { allAlpha = false; break; }
        if (!allAlpha) {
            std::cout << COLOR_RED << P << "Letters only.\n" << COLOR_RESET;
            continue;
        }
        return input;
    }
}


static void renderWordleBoard(const std::vector<std::string>& guesses,
                               const std::string& target, int wordLength) {
    // title(6) + rows(5×2=10) + letters(3) + prompt(2) = ~21 lines
    clearAndCenterV(21);
    printWordleTitle(wordLength);
    for (int i = 0; i < MAX_GUESSES; i++) {
        if (i < (int)guesses.size()) printGuessRow(guesses[i], target);
        else                         printEmptyRow(wordLength);
        std::cout << "\n";
    }
    printUsedLetters(guesses, target, wordLength);
}

// ─── ENTRY POINT ─────────────────────────────────────────────────────────────

/*
 * runWordle
 * Runs the Wordle minigame. Picks a random word of length wordLength.
 * Input:  wordLength - 4 (easy), 5 (normal), 6 (hard)
 * Output: true if player won, false if they ran out of guesses
 */
bool runWordle(int wordLength) {
    const std::vector<std::string>* wordList =
        (wordLength == 4) ? &WORDS_4 :
        (wordLength == 5) ? &WORDS_5 : &WORDS_6;

    std::string target = (*wordList)[rand() % (int)wordList->size()];
    std::vector<std::string> guesses;
    bool won = false;

    while ((int)guesses.size() < MAX_GUESSES && !won) {
        renderWordleBoard(guesses, target, wordLength);
        std::string P = hpad(38);
        std::cout << COLOR_DIM << P << "Guess " << (guesses.size() + 1)
                  << " of " << MAX_GUESSES << COLOR_RESET;

        std::string guess = getWordleGuess(wordLength);
        if (g_minigameForfeited) break;
        guesses.push_back(guess);
        if (guess == target) won = true;
    }

    renderWordleBoard(guesses, target, wordLength);

    
clearScreen();
std::cout << "\n";
showWordleEndScreen(won, target);
    
    std::cout << "\n" << COLOR_DIM << hpad(30) << "Press any key to continue..." << COLOR_RESET;
    std::cout.flush();
    char dummy;
    read(STDIN_FILENO, &dummy, 1);

    return won;
}
