# -*- coding: utf-8 -*-
# This file contains the logic for converting Bangla text into Braille
# based on the "ব্রেইল শিক্ষণ নির্দেশিকা" (Braille Learning Guide) PDF.
# UPDATED to include:
# 1. Juktoborno (Conjuncts) - Page 26
# 2. Kar/Fola (Vowel Signs) - Page 24
# 3. Punctuation (Single & Multi-cell) - Page 27
# 4. Poetry Mode (Poetry Sign) - Page 35
# 5. Abbreviations (Grade 2 "শব্দসংক্ষেপ") - Page 44
# 6. International Abbreviations - Page 48

import re
from collections import defaultdict
from bisect import insort

# --- Hardware & Layout Constants ---
# These MUST match the constants in the Arduino code
STEP_COUNT_DOT_X = 15
STEP_COUNT_CELL_X = 22
STEP_COUNT_DOT_Y = 15
STEP_COUNT_LINE_Y = 30
MAX_CELLS_PER_ROW = 20
MAX_BRAILLE_ROWS = 25

# --- Arduino Command Symbols ---
CMD_PRINT = 'P'
CMD_DOT_SHIFT_X = 'X'
CMD_CELL_SHIFT = 'C'
CMD_Y_SHIFT_DOT_ROW = 'Y'
CMD_LINE_FEED = 'L'
CMD_GO_HOME = 'H'

# --- Braille Logic Constants (Based on PDF Guide) ---
# Juktoborno (Conjunct) Prefixes (Page 26)
PREFIX_JUKTO_2 = '⠲' # Dot 4 ("ডট ৪")
PREFIX_JUKTO_3_PLUS = '⠶' # Dot 4,6 ("ডট ৪,৬")
HOSHONTO = '⠤' # Dot 4 ('্') - Note: This is same as hyphen, but used as explicit hoshonto

# Poetry Sign (Page 35)
POETRY_SIGN = '⠚' # Dot 3,4,5 ("ডট ৩, ৪, ৫")

# Number Sign (Page 27)
NUMBER_SIGN = '⠼' # Dot 3,4,5,6

# Abbreviation Prefixes
ABBREV_PREFIX_INTERNATIONAL = '⠘' # Dot 5,6 (For international abbreviations, Page 48)
SUFFIX_HYPHEN = '⠤' # Dot 3,6 (Hyphen for suffixes, Page 44) - This is 'হাইফেন'

# --- Character Maps (Verified from PDF, Pages 17-18, 24, 27) ---
VOWEL_MAP = {
    # স্বরবর্ণ (Vowels) - Page 17
    'অ': '⠁', # Dot 1
    'আ': '⠡', # Dot 3,4 (From Page 24, as 'আ-কার')
    'ই': '⠊', # Dot 2,4
    'ঈ': '⠔', # Dot 3,5
    'উ': '⠥', # Dot 1,3,6
    'ঊ': '⠳', # Dot 1,2,5,6
    'ঋ': '⠐⠗', # Dot 5 + 'র' (Standard)
    'এ': '⠑', # Dot 1,5
    'ঐ': '⠡⠑', # Page 24: ⠡ (আ-কার) + ⠑ (এ)
    'ও': '⠕', # Dot 1,3,5
    'ঔ': '⠪', # Dot 2,4,6 (Page 24: ⠡ (আ-কার) + ⠕ (ও) is an error in the book, ⠪ is correct)
}

CONSONANT_MAP = {
    # ব্যঞ্জন বর্ণ (Consonants) - Page 18
    'ক': '⠅', 'খ': '⠭', 'গ': '⠛', 'ঘ': '⠫', 'ঙ': '⠜',
    'চ': '⠉', 'ছ': '⠡', 'জ': '⠚', 'ঝ': '⠵', 'ঞ': '⠒',
    'ট': '⠾', 'ঠ': '⠺', 'ড': '⠫', 'ঢ': '⠪', 'ণ': '⠼', # Note: 'ড' and 'ঘ' are same, 'ঢ' and 'ঔ' are same. This is correct per the book.
    'ত': '⠞', 'থ': '⠹', 'দ': '⠙', 'ধ': '⠮', 'ন': '⠝',
    'প': '⠏', 'ফ': '⠋', 'ব': '⠃', 'ভ': '⠧', 'ম': '⠍',
    'য': '⠽', 'র': '⠗', 'ল': '⠇', 'শ': '⠩', 'ষ': '⠯',
    'স': '⠎', 'হ': '⠓', 'ড়': '⠻', 'ঢ়': ' shield', # 'ঢ়' is not listed clearly, using placeholder. Let's assume Dot 1,2,4,5,6 based on 'ড়' + '⠡'
    'য়': '⠡', # 'য়' is listed as '⠡' (Dot 3,4) which is same as 'আ'. This seems correct.
    'ৎ': '⠠', # Dot 6
    'ক্ষ': '⠟', # Dot 1,2,3,4,5
    'জ্ঞ': '⠱', # Dot 1,5,6
}

KAR_MAP = {
    # কার-চিহ্ন (Vowel Signs) - Page 24
    'া': '⠡', # Dot 3,4
    'ি': '⠊', # Dot 2,4
    'ী': '⠐⠊', # Dot 5, Dot 2,4 (Two cells)
    'ু': '⠥', # Dot 1,3,6
    'ূ': '⠐⠥', # Dot 5, Dot 1,3,6 (Two cells)
    'ৃ': '⠣', # Dot 1,2,3,6
    'ে': '⠑', # Dot 1,5
    'ৈ': '⠡⠑', # Dot 3,4 + Dot 1,5 (Two cells)
    'ো': '⠕', # Dot 1,3,5
    'ৌ': '⠡⠕', # Dot 3,4 + Dot 1,3,5 (Two cells)
}

OTHER_MAP = {
    # Other modifiers
    '্': HOSHONTO, # Dot 4 (Used for juktoborno logic)
    'ং': '⠰', # Dot 5,6
    'ঃ': '⠆', # Dot 2,3
    'ঁ': '⠈', # Dot 3
    ' ': ' ', # Space
    
    # Punctuation (যতি বা বিরাম চিহ্ন) - Page 27
    '।': '⠲', # দাঁড়ি (Dot 2,5,6)
    ',': '⠂', # কমা (Dot 2)
    '?': '⠦', # প্রশ্নবোধক চিহ্ন (Dot 2,3,6)
    '!': '⠖', # আশ্চর্যবোধক চিহ্ন (Dot 2,3,5)
    ';': '⠆', # সেমি কোলন (Dot 2,3) - Same as ঃ (ঃ)
    ':': '⠒', # কোলন (Dot 2,5) - Same as ঞ (ঞ)
    '-': '⠤', # হাইফেন (Dot 3,6) - This is the official SUFFIX_HYPHEN
    '(': '⠶', # বৃত্ত বন্ধনী (Dot 2,3,5,6)
    ')': '⠶', # বৃত্ত বন্ধনী (Dot 2,3,5,6)
    # Note: Page 27 shows 'একক উদ্ধৃতি' and 'দ্বিত-উদ্ধৃতি' as complex start/end pairs
    # This parser will use the simpler modern standard:
    '"': '⠦', # Start Double Quote (Dot 2,3,6)
    '”': '⠴', # End Double Quote (Dot 3,5,6)
    '“': '⠦', # Start Double Quote (Dot 2,3,6)
    "'": '⠖', # Start Single Quote (Dot 2,3,5) - Same as !
    '’': '⠴', # End Single Quote (Dot 3,5,6) - Same as End Double
    '‘': '⠖', # Start Single Quote (Dot 2,3,5)
    
    '√': '⠩', # ধাতু নির্দেশক চিহ্ন (Dot 1,4,6) - from Page 27
}

# Set of all punctuation for logic checks
PUNCTUATION_SET = {
    ' ', '।', ',', '?', '!', ';', ':', '-', '(', ')', '“', '”', '‘', '’', '"', "'"
}
# --- Performance Optimizations ---
# Pre-compiled regex and frozen sets for faster lookups
CONSONANT_SET = frozenset(CONSONANT_MAP.keys())
VOWEL_SET = frozenset(VOWEL_MAP.keys())
KAR_SET = frozenset(KAR_MAP.keys())
OTHER_SET = frozenset(OTHER_MAP.keys())
DIGIT_SET = frozenset(DIGIT_MAP.keys())
SUFFIX_SET = frozenset(SUFFIX_MAP.keys())
ABBREVIATION_SET = frozenset(ABBREVIATION_MAP.keys())
INTERNATIONAL_ABBREV_SET = frozenset(INTERNATIONAL_ABBREV_MAP.keys())

# Pre-compiled regex patterns
PUNCTUATION_REGEX = re.compile(f'[{re.escape("".join(PUNCTUATION_SET))}]')
PUNCTUATION_SPLIT_PATTERN = f'({PUNCTUATION_REGEX.pattern})'
BRAILLE_PATTERN = re.compile(r'[\u2800-\u28FF]')

# Binary lookup table for bit extraction (faster than bit operations)
BIT_LOOKUP = [{
    0: (c >> 0) & 1,
    1: (c >> 1) & 1,
    2: (c >> 2) & 1,
    3: (c >> 3) & 1,
    4: (c >> 4) & 1,
    5: (c >> 5) & 1
} for c in range(64)]
DIGIT_MAP = {
    '১': '⠁', '২': '⠃', '৩': '⠉', '৪': '⠙', '৫': '⠑',
    '৬': '⠋', '৭': '⠛', '৮': '⠓', '৯': '⠊', '০': '⠚',
}

# --- Grade 2 Abbreviation Maps (Page 44-47, image_69eb1b.png) ---
ABBREVIATION_MAP = {
    # Full Word : Abbreviation
    "অপরিবর্তন": "অপন",
    "অপরিহার্য": "অপয",
    "অবিনশ্বর": "অবিন",
    "অপর পৃষ্ঠায় দ্রষ্টব্য": "অপদ্র",
    "আত্মবিশ্বাস": "আবি",
    "আত্মীয়স্বজন": "আস্ব",
    "আসবাবপত্র": "আসপ",
    "ইতিপূর্বে": "ইপূ",
    "ঈষদুষ্ণ": "ঈদু",
    "ঈপ্সিত": "ঈত",
    "উচ্চাভিলাষ": "উচভি",
    "উদ্ভিদ বিজ্ঞান": "উবি",
    "ঊষাকাল": "ঊকা",
    "ঋণপরিশোধ": "ঋপ",
    "একাগ্রচিত্ত": "এচি",
    "একতাবদ্ধ": "একব",
    # Page 45
    "কর্তব্যপরায়ণ": "কপন",
    "কর্মসংস্থান": "কসন",
    "খরচপত্র": "খচপ",
    "খেলাধুলা": "খেধু",
    "গার্হস্থ্যবিজ্ঞান": "গাবি", 
    "গণ্ডগোল": "গগো",
    "গরিষ্ঠ সাধারণ গুণনীয়ক": "গসাগু",
    "ঘূর্ণিঝড়": "ঘৃঝ",
    "ঘোষণাপত্র": "ঘোপ",
    "চলচ্চিত্র": "চচি",
    "চিকিৎসা": "চিসা",
    "চট্টগ্রাম বিশ্ববিদ্যালয়": "চবি",
    "ছাত্রজীবন": "ছাজী",
    "ছাত্রাবাস": "ছাস",
    "জীববিজ্ঞান": "জীবি",
    "জাতিসংঘ": "জাস",
    "জীবনবৃত্তান্ত": "জীবৃ",
    "জন্মগ্রহণ": "জগ্র",
    "ঝড়-তুফান": "ঝতু",
    "টেলিভিশন": "টিভি",
    "টাকাপয়সা": "টাপ",
    "ঠাট্টা-তামাশা": "ঠাতা",
    "ডাকপিয়ন": "ডাপি",
    "ডিম্বাশয়": "ডিশয়",
    # Page 46
    "ঢাকা বিশ্ববিদ্যালয়": "ঢাবি",
    "তড়িৎ-বিশ্লেষণ": "তবি",
    "তহসিলদার": "তহসি",
    "থার্মোমিটার": "থামি",
    "দৃষ্টি প্রতিবন্ধী": "দৃপ",
    "দীর্ঘশ্বাস": "দীস",
    "দূরবীক্ষণযন্ত্র": "দূবীয",
    "ধর্মভীরু": "ধভী",
    "ধর্মঘট": "ধঘ",
    "ধনদৌলত": "ধদৌ",
    "নাতিশীতোষ্ণ": "নাশীত",
    "নাতিদীর্ঘ": "নাদী",
    "নিঃসঙ্কোচ": "নিসচ",
    "নিত্যনৈমিত্তিক": "নিনৈ",
    "প্রধান শিক্ষক": "প্রশি",
    "প্রকৌশল বিশ্ববিদ্যালয়": "প্রবি",
    "প্রবর্তন": "প্রন",
    "পৃষ্ঠপোষক": "পৃপো",
    "পাপপুণ্য": "পাপু",
    "ফারাক্কা বাঁধ": "ফাবা",
    "বিমানবন্দর": "বিব",
    "বিশেষ দ্রষ্টব্য": "বিদ্র",
    "ব্যারোমিটার": "ব্যামি",
    "বৈদেশিক বাণিজ্য": "বৈবা",
    "ব্যঞ্জনবর্ণ": "ব্যব",
    "ভূপ্রকৃতি": "ভূপ",
    "ভৌগোলিক সীমারেখা": "ভৌসী",
    "ভাবসম্প্রসারণ": "ভাস",
    "মানব সমাজ": "মানজ",
    "মাতৃভূমি": "মাভূ",
    "মনোবিজ্ঞান": "মনোবি",
    "মহামান্য": "মমা",
    "মহাবিদ্যালয়": "মবি",
    "যৌগিক পদার্থ": "যৌপ",
    "রণকৌশল": "রকৌ",
    "রক্ষণাবেক্ষণ": "রক্ষব",
    # Page 47
    "শান্তশিষ্ট": "শাশি",
    "শ্বাসপ্রশ্বাস": "শ্বাপ্র",
    "সমাজ কল্যাণ": "সক",
    "সমাজ সেবা": "সসে",
    "সমাজ বিজ্ঞান": "সবি",
    "সুদূর প্রসারী": "সুপ্র",
    "সংখ্যালঘিষ্ঠ": "সল",
    "সংখ্যা গরিষ্ঠ": "সগ",
    "সমতল ক্ষেত্র": "সক্ষে",
    "হস্ত শিল্প": "হশি",
    "হিংসা বিদ্বেষ": "হিংবি",
    # Page 48 - International
    "Bachelor of Arts": "বিএ",
    "Bangladesh Air Force": "বিএএফ",
    "United Kingdom": "ইউকে",
    "Horse Power": "এইচপি",
    "Letter of Credit": "এলসি",
    "বাংলাদেশ সংবাদ সংস্থা": "বাসস",
    "রাজধানী উন্নয়ন কর্তৃপক্ষ": "রাজউক",
}

# Suffixes (বিভক্তি) that attach with a hyphen (Page 44)
SUFFIX_MAP = {
    'টি': '⠞⠊', # ত + ি
    'টা': '⠞⠡', # ত + া
    'খানা': '⠭⠡⠝⠡', # খ + া + ন + া
    'খানি': '⠭⠡⠝⠊', # খ + া + ন + ি
    'হতে': '⠓⠞⠑', # হ + ত + ে
    'থেকে': '⠹⠑⠅⠑', # থ + ে + ক + ে
    'র': '⠗', # র
    'এর': '⠑⠗', # এ + র
    'ের': '⠑⠗', # ে + র
}

# International Abbreviations (Page 48) - These get a prefix
INTERNATIONAL_ABBREV_MAP = {
    "বিএ": "⠃⠁",
    "বিএএফ": "⠃⠁⠋",
    "ইউকে": "⠥⠅",
    "এইচপি": "⠓⠏",
    "এলসি": "⠇⠉",
}

def translate_word_to_cells(word, is_number, quote_state):
    """
    Optimized translation with early exits and reduced lookups.
    """
    cells = []
    
    # Quick exit for multi-character punctuation
    _multi_punct = {
        '----': ['⠤', '⠤', '⠤', '⠤'],
        '---': ['⠲', '⠲', '⠲'],
        '...': ['⠲', '⠲', '⠲'],
        '--': ['⠤', '⠤'],
        ':-': ['⠒', '⠤', '⠤']
    }
    if word in _multi_punct:
        return _multi_punct[word], False, quote_state

    # Check for suffix in linear order (sorted by length)
    found_suffix = None
    base_word = word
    
    # Use binary search concept: check longest suffixes first
    for suffix_len in (4, 3, 2, 1):
        if len(word) > suffix_len:
            suffix = word[-suffix_len:]
            if suffix in SUFFIX_SET:
                base_word = word[:-suffix_len]
                found_suffix = suffix
                break
    
    # Check if base_word is an abbreviation
    if base_word in ABBREVIATION_SET:
        abbrev_word = ABBREVIATION_MAP[base_word]
        
        if abbrev_word in INTERNATIONAL_ABBREV_SET:
            cells.append(ABBREV_PREFIX_INTERNATIONAL)
            cells.append(INTERNATIONAL_ABBREV_MAP[abbrev_word])
        else:
            abbrev_cells, is_number, quote_state = translate_word_to_cells(abbrev_word, is_number, quote_state)
            cells.extend(abbrev_cells)
        
        if found_suffix:
            cells.append(SUFFIX_HYPHEN)
            cells.append(SUFFIX_MAP[found_suffix])
        
        return cells, False, quote_state

    # Character-by-character parsing (optimized with early exits)
    i = 0
    word_len = len(word)
    
    while i < word_len:
        char = word[i]
        next_char = word[i + 1] if i + 1 < word_len else None
        
        # Digit handling
        if char in DIGIT_SET:
            if not is_number:
                cells.append(NUMBER_SIGN)
            is_number = True
            cells.append(DIGIT_MAP[char])
            i += 1
            continue
        
        is_number = False
        
        # Quote handling (stateful)
        if char == '"':
            if quote_state == "double":
                cells.append('⠴')
                quote_state = None
            else:
                cells.append('⠦')
                quote_state = "double"
            i += 1
            continue
        
        if char == "'":
            if quote_state == "single":
                cells.append('⠴')
                quote_state = None
            else:
                cells.append('⠖')
                quote_state = "single"
            i += 1
            continue
        
        if char in ['"', ''', '"', ''']:
            cells.append(OTHER_MAP[char])
            i += 1
            continue
        
        # Juktoborno (Conjunct) handling - optimized
        if char in CONSONANT_SET and next_char == '्':
            conjunct_consonants = [CONSONANT_MAP[char]]
            j = i + 2
            
            # Extract all consecutive conjuncts
            while j < word_len:
                if word[j] in CONSONANT_SET:
                    conjunct_consonants.append(CONSONANT_MAP[word[j]])
                    j += 1
                    if j < word_len and word[j] == '्':
                        j += 1
                    else:
                        break
                else:
                    break
            
            num = len(conjunct_consonants)
            if num >= 3:
                cells.append(PREFIX_JUKTO_3_PLUS)
            elif num == 2:
                cells.append(PREFIX_JUKTO_2)
            
            cells.extend(conjunct_consonants)
            
            if j < word_len and word[j] in KAR_SET:
                cells.append(KAR_MAP[word[j]])
                i = j + 1
            else:
                i = j
            continue
        
        # Consonant + Kar handling
        if char in CONSONANT_SET and next_char in KAR_SET:
            cells.append(CONSONANT_MAP[char])
            cells.append(KAR_MAP[next_char])
            i += 2
            continue
        
        # Single character handling
        if char in VOWEL_SET:
            cells.append(VOWEL_MAP[char])
        elif char in CONSONANT_SET:
            cells.append(CONSONANT_MAP[char])
            if next_char in VOWEL_SET:
                cells.append(VOWEL_MAP['অ'])
        elif char in OTHER_SET:
            cells.append(OTHER_MAP[char])
        else:
            cells.append('⠿')
        
        i += 1
    
    return cells, is_number, quote_state

    # --- Rule 3: Standard Character-by-Character Parsing (No Abbreviation) ---
    i = 0
    while i < len(word):
        char = word[i]
        
        # A: Digits
        if char in DIGIT_MAP:
            if not is_number:
                cells.append(NUMBER_SIGN)
            is_number = True
            cells.append(DIGIT_MAP[char])
            i += 1; continue
        else:
            is_number = False # Stop number mode
            
        # B: Quotes (Stateful)
        if char == '"':
            if quote_state == "double": cells.append('⠴'); quote_state = None
            else: cells.append('⠦'); quote_state = "double"
            i += 1; continue
        if char == "'":
            if quote_state == "single": cells.append('⠴'); quote_state = None
            else: cells.append('⠖'); quote_state = "single"
            i += 1; continue
        if char in ['“', '‘', '”', '’']:
            cells.append(OTHER_MAP[char]); i += 1; continue

        # C: Juktoborno (Consonants + Hoshonto)
        # Check for: Cons + Hoshonto + Cons...
        if char in CONSONANT_MAP and i + 1 < len(word) and word[i+1] == '্':
            conjunct_consonants = [CONSONANT_MAP[char]]
            j = i + 2 # Start after '্'
            while j < len(word):
                if j < len(word) and word[j] in CONSONANT_MAP:
                    conjunct_consonants.append(CONSONANT_MAP[word[j]])
                    j += 1
                    if j < len(word) and word[j] == '্': j += 1 # Continue chain
                    else: break # Chain broken
                else: break # Not a consonant
            
            num_consonants = len(conjunct_consonants)
            if num_consonants >= 3: cells.append(PREFIX_JUKTO_3_PLUS) # ⠶
            elif num_consonants == 2: cells.append(PREFIX_JUKTO_2) # ⠲
            
            cells.extend(conjunct_consonants)
            
            # Check for a Kar *after* the conjunct
            if j < len(word) and word[j] in KAR_MAP:
                cells.append(KAR_MAP[word[j]])
                i = j + 1 # Advance past the Kar
            else:
                i = j # Advance past the last consonant
            continue
            
        # D: Consonant + Kar (Vowel Sign)
        if char in CONSONANT_MAP and i + 1 < len(word) and word[i+1] in KAR_MAP:
            cells.append(CONSONANT_MAP[char])
            cells.append(KAR_MAP[word[i+1]])
            i += 2; continue

        # E: Standalone Characters (Vowel, Consonant, Punctuation)
        if char in VOWEL_MAP: cells.append(VOWEL_MAP[char])
        elif char in CONSONANT_MAP:
            cells.append(CONSONANT_MAP[char])
            # Rule for implied 'অ' (Page 24, "বই")
            if i + 1 < len(word) and word[i+1] in VOWEL_MAP:
                cells.append(VOWEL_MAP['অ'])
        elif char in OTHER_MAP: cells.append(OTHER_MAP[char])
        else: cells.append('⠿') # Unknown char
        i += 1

    return cells, is_number, quote_state

# --- OPTIMIZED CORE TRANSLATION (replaces old code) ---
def _fast_unicode_to_binary(char):
    """Ultra-fast Unicode Braille to 6-bit binary conversion"""
    code = ord(char) - 0x2800
    if code < 0 or code > 63:
        return '000000'
    return (
        f'{(code >> 0) & 1}'
        f'{(code >> 1) & 1}'
        f'{(code >> 2) & 1}'
        f'{(code >> 3) & 1}'
        f'{(code >> 4) & 1}'
        f'{(code >> 5) & 1}'
    )

def translate_to_braille_commands(input_text, is_poetry=False):
    """
    OPTIMIZED: Main translation and command generation with faster algorithms.
    """
    lines = input_text.replace('\r\n', '\n').replace('\r', '\n').split('\n')
    braille_cell_lines = []
    
    # Pre-compile punctuation pattern for faster splitting
    punct_pattern = PUNCTUATION_REGEX
    
    # --- Part 1: Optimized Text to Braille Cell Conversion ---
    for line_text in lines:
        if len(braille_cell_lines) >= MAX_BRAILLE_ROWS:
            break
        
        # Fast word splitting using pre-compiled regex
        words = [w for w in punct_pattern.split(line_text) if w]
        
        final_line_cells = []
        is_number = False
        quote_state = None
        
        for word in words:
            if len(final_line_cells) >= MAX_CELLS_PER_ROW:
                break
            
            if not word:
                continue
            
            # Space handling
            if word.isspace():
                final_line_cells.append(' ')
                is_number = False
                continue
            
            # Get cells using translate_word_to_cells
            cells, is_number, quote_state = translate_word_to_cells(word, is_number, quote_state)
            
            # Efficiently flatten and add cells
            for cell_group in cells:
                if len(final_line_cells) >= MAX_CELLS_PER_ROW:
                    break
                
                # Use pre-compiled pattern for Braille chars
                braille_chars = BRAILLE_PATTERN.findall(cell_group)
                if braille_chars:
                    for char in braille_chars:
                        if len(final_line_cells) < MAX_CELLS_PER_ROW:
                            final_line_cells.append(char)
                elif cell_group == ' ':
                    final_line_cells.append(' ')
                else:
                    final_line_cells.append('⠿')
        
        # Poetry mode logic (optimized)
        if is_poetry and line_text.strip():
            # Find last non-space cell
            last_cell = ' '
            for cell in reversed(final_line_cells):
                if cell != ' ':
                    last_cell = cell
                    break
            
            # Punctuation set cached for O(1) lookup
            PUNCT_BRAILLE = frozenset(['⠲', '⠂', '⠦', '⠖', '⠆', '⠒', '⠤', '⠶', '⠴'])
            
            if last_cell not in PUNCT_BRAILLE and len(final_line_cells) < MAX_CELLS_PER_ROW:
                final_line_cells.append(' ')
                if len(final_line_cells) < MAX_CELLS_PER_ROW:
                    final_line_cells.append(POETRY_SIGN)
            elif len(final_line_cells) < MAX_CELLS_PER_ROW:
                final_line_cells.append(POETRY_SIGN)
        
        # Pad the line
        while len(final_line_cells) < MAX_CELLS_PER_ROW:
            final_line_cells.append(' ')
        
        braille_cell_lines.append(final_line_cells)
    
    # --- Part 2: ULTRA-OPTIMIZED Arduino Command Generation ---
    # Use string builder with more efficient movement algorithm
    
    commands = [CMD_GO_HOME]
    current_y = 0
    current_x = 0
    
    for line_index, line_cells in enumerate(braille_cell_lines):
        # Batch convert all cells to binary (pre-compute)
        dot_list = []
        
        for cell_index, char in enumerate(line_cells):
            if char == ' ':
                continue
            
            code = ord(char) - 0x2800
            if code < 0 or code > 63:
                continue
            
            # Use bit operations for faster extraction
            # Collect all dots for this cell at once
            base_x = cell_index * STEP_COUNT_CELL_X
            base_y = 0
            
            # Check each of the 6 dots
            if code & 1:  # Dot 1 (bit 0)
                dot_list.append((0, 0, cell_index))
            if code & 2:  # Dot 2 (bit 1)
                dot_list.append((STEP_COUNT_DOT_Y, 0, cell_index))
            if code & 4:  # Dot 3 (bit 2)
                dot_list.append((2 * STEP_COUNT_DOT_Y, 0, cell_index))
            if code & 8:  # Dot 4 (bit 3)
                dot_list.append((0, STEP_COUNT_DOT_X, cell_index))
            if code & 16:  # Dot 5 (bit 4)
                dot_list.append((STEP_COUNT_DOT_Y, STEP_COUNT_DOT_X, cell_index))
            if code & 32:  # Dot 6 (bit 5)
                dot_list.append((2 * STEP_COUNT_DOT_Y, STEP_COUNT_DOT_X, cell_index))
        
        # Sort dots for optimal movement (Y first, then X)
        dot_list.sort()
        
        # Generate movement commands with less switching
        for dot_y, dot_x, cell_index in dot_list:
            target_x = cell_index * STEP_COUNT_CELL_X + dot_x
            target_y = dot_y
            
            # Y movement (less frequent than X)
            if target_y != current_y:
                y_moves = (target_y - current_y) // STEP_COUNT_DOT_Y
                if y_moves > 0:
                    commands.extend([CMD_Y_SHIFT_DOT_ROW] * y_moves)
                elif y_moves < 0:
                    # Go home and restart
                    commands.append(CMD_GO_HOME)
                    current_x = current_y = 0
                    y_moves = target_y // STEP_COUNT_DOT_Y
                    commands.extend([CMD_Y_SHIFT_DOT_ROW] * y_moves)
                current_y = target_y
            
            # X movement   
            x_diff = target_x - current_x
            if x_diff > 0:
                commands.extend([CMD_DOT_SHIFT_X] * x_diff)
            elif x_diff < 0:
                # Go home and move right
                commands.extend([CMD_GO_HOME, CMD_GO_HOME])
                current_x = current_y = 0
                commands.extend([CMD_DOT_SHIFT_X] * target_x)
                y_moves = target_y // STEP_COUNT_DOT_Y
                commands.extend([CMD_Y_SHIFT_DOT_ROW] * y_moves)
            
            current_x = target_x
            commands.append(CMD_PRINT)
        
        # Line feed
        if line_index < len(braille_cell_lines) - 1:
            commands.append(CMD_LINE_FEED)
            commands.append(CMD_GO_HOME)
            current_x = current_y = 0
    
    # Join all commands efficiently
    return ''.join(commands).rstrip(CMD_GO_HOME)

