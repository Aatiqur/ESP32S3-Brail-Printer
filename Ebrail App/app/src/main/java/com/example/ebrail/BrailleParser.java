package com.example.ebrail;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class BrailleParser {

    public static final int STEP_COUNT_DOT_X = 15;
    public static final int STEP_COUNT_CELL_X = 22;
    public static final int STEP_COUNT_DOT_Y = 15;
    public static final int STEP_COUNT_LINE_Y = 30;
    public static final int MAX_CELLS_PER_ROW = 20;
    public static final int MAX_BRAILLE_ROWS = 25;

    public static final String CMD_PRINT = "P";
    public static final String CMD_DOT_SHIFT_X = "X";
    public static final String CMD_CELL_SHIFT = "C";
    public static final String CMD_Y_SHIFT_DOT_ROW = "Y";
    public static final String CMD_LINE_FEED = "L";
    public static final String CMD_GO_HOME = "H";

    public static final String PREFIX_JUKTO_2 = "⠲";
    public static final String PREFIX_JUKTO_3_PLUS = "⠶";
    public static final String NUMBER_SIGN = "⠼";
    public static final String ABBREV_PREFIX_INTERNATIONAL = "⠘";
    public static final String SUFFIX_HYPHEN = "⠤";

    private static final Map<Character, String> VOWEL_MAP = new HashMap<>();
    private static final Map<Character, String> CONSONANT_MAP = new HashMap<>();
    private static final Map<Character, String> KAR_MAP = new HashMap<>();
    private static final Map<Character, String> OTHER_MAP = new HashMap<>();
    private static final Map<Character, String> DIGIT_MAP = new HashMap<>();
    private static final Map<String, String> ABBREVIATION_MAP = new HashMap<>();
    private static final Map<String, String> SUFFIX_MAP = new HashMap<>();
    private static final Map<String, String> INTERNATIONAL_ABBREV_MAP = new HashMap<>();

    static {
        VOWEL_MAP.put('অ', "⠁"); VOWEL_MAP.put('আ', "⠡"); VOWEL_MAP.put('ই', "⠊");
        VOWEL_MAP.put('ঈ', "⠔"); VOWEL_MAP.put('উ', "⠥"); VOWEL_MAP.put('ঊ', "⠳");
        VOWEL_MAP.put('ঋ', "⠐⠗"); VOWEL_MAP.put('এ', "⠑"); VOWEL_MAP.put('ঐ', "⠡⠑");
        VOWEL_MAP.put('ও', "⠕"); VOWEL_MAP.put('ঔ', "⠪");

        CONSONANT_MAP.put('ক', "⠅"); CONSONANT_MAP.put('খ', "⠭"); CONSONANT_MAP.put('গ', "⠛");
        CONSONANT_MAP.put('ঘ', "⠫"); CONSONANT_MAP.put('ঙ', "⠜"); CONSONANT_MAP.put('চ', "⠉");
        CONSONANT_MAP.put('ছ', "⠡"); CONSONANT_MAP.put('জ', "⠚"); CONSONANT_MAP.put('ঝ', "⠵");
        CONSONANT_MAP.put('ঞ', "⠒"); CONSONANT_MAP.put('ট', "⠾"); CONSONANT_MAP.put('ঠ', "⠺");
        CONSONANT_MAP.put('ড', "⠫"); CONSONANT_MAP.put('ঢ', "⠪"); CONSONANT_MAP.put('ণ', "⠼");
        CONSONANT_MAP.put('ত', "⠞"); CONSONANT_MAP.put('থ', "⠹"); CONSONANT_MAP.put('দ', "⠙");
        CONSONANT_MAP.put('ধ', "⠮"); CONSONANT_MAP.put('ন', "⠝"); CONSONANT_MAP.put('প', "⠏");
        CONSONANT_MAP.put('ফ', "⠋"); CONSONANT_MAP.put('ব', "⠃"); CONSONANT_MAP.put('ভ', "⠧");
        CONSONANT_MAP.put('ম', "⠍"); CONSONANT_MAP.put('য', "⠽"); CONSONANT_MAP.put('র', "⠗");
        CONSONANT_MAP.put('ল', "⠇"); CONSONANT_MAP.put('শ', "⠩"); CONSONANT_MAP.put('ষ', "⠯");
        CONSONANT_MAP.put('স', "⠎"); CONSONANT_MAP.put('হ', "⠓"); CONSONANT_MAP.put('ড়', "⠻");
        CONSONANT_MAP.put('ঢ়', "⠡"); CONSONANT_MAP.put('য়', "⠡"); CONSONANT_MAP.put('ৎ', "⠠");
        // 'ক্ষ' and 'জ্ঞ' are conjuncts, not single chars, moving to ABBREVIATION_MAP

        KAR_MAP.put('া', "⠡"); KAR_MAP.put('ি', "⠊"); KAR_MAP.put('ী', "⠐⠊");
        KAR_MAP.put('ু', "⠥"); KAR_MAP.put('ূ', "⠐⠥"); KAR_MAP.put('ৃ', "⠣");
        KAR_MAP.put('ে', "⠑"); KAR_MAP.put('ৈ', "⠡⠑"); KAR_MAP.put('ো', "⠕");
        KAR_MAP.put('ৌ', "⠡⠕");

        OTHER_MAP.put('্', "⠤"); OTHER_MAP.put('ং', "⠰"); OTHER_MAP.put('ঃ', "⠆");
        OTHER_MAP.put('ঁ', "⠈"); OTHER_MAP.put(' ', " "); OTHER_MAP.put('।', "⠲");
        OTHER_MAP.put(',', "⠂"); OTHER_MAP.put('?', "⠦"); OTHER_MAP.put('!', "⠖");
        OTHER_MAP.put(';', "⠆"); OTHER_MAP.put(':', "⠒"); OTHER_MAP.put('-', "⠤");
        OTHER_MAP.put('(', "⠶"); OTHER_MAP.put(')', "⠶"); OTHER_MAP.put('"', "⠦");
        OTHER_MAP.put('”', "⠴"); OTHER_MAP.put('“', "⠦"); OTHER_MAP.put('\'', "⠖");
        OTHER_MAP.put('’', "⠴"); OTHER_MAP.put('‘', "⠖");

        DIGIT_MAP.put('১', "⠁"); DIGIT_MAP.put('২', "⠃"); DIGIT_MAP.put('৩', "⠉");
        DIGIT_MAP.put('৪', "⠙"); DIGIT_MAP.put('৫', "⠑"); DIGIT_MAP.put('৬', "⠋");
        DIGIT_MAP.put('৭', "⠛"); DIGIT_MAP.put('৮', "⠓"); DIGIT_MAP.put('৯', "⠊");
        DIGIT_MAP.put('০', "⠚");
        
        // Latin/English Basic Fallbacks
        String engAlpha = "abcdefghijklmnopqrstuvwxyz";
        String engBraille = "⠁⠃⠉⠙⠑⠋⠛⠓⠊⠚⠅⠇⠍⠝⠕⠏⠟⠗⠎⠞⠥⠧⠺⠭⠽⠵";
        for (int i=0; i<engAlpha.length(); i++) {
            CONSONANT_MAP.put(engAlpha.charAt(i), String.valueOf(engBraille.charAt(i)));
            CONSONANT_MAP.put(Character.toUpperCase(engAlpha.charAt(i)), String.valueOf(engBraille.charAt(i)));
        }
        String engDigits = "1234567890";
        for(int i=0; i<engDigits.length(); i++) {
            DIGIT_MAP.put(engDigits.charAt(i), String.valueOf(engBraille.charAt(i)));
        }

        SUFFIX_MAP.put("টি", "⠞⠊"); SUFFIX_MAP.put("টা", "⠞⠡");
        SUFFIX_MAP.put("খানা", "⠭⠡⠝⠡"); SUFFIX_MAP.put("খানি", "⠭⠡⠝⠊");
        SUFFIX_MAP.put("হতে", "⠓⠞⠑"); SUFFIX_MAP.put("থেকে", "⠹⠑⠅⠑");
        SUFFIX_MAP.put("র", "⠗"); SUFFIX_MAP.put("এর", "⠑⠗"); SUFFIX_MAP.put("ের", "⠑⠗");

        // Grade 2 Abbreviations (Page 44-47)
        ABBREVIATION_MAP.put("অপরিবর্তন", "অপন");
        ABBREVIATION_MAP.put("অপরিহার্য", "অপয");
        ABBREVIATION_MAP.put("অবিনশ্বর", "অবিন");
        ABBREVIATION_MAP.put("অপর পৃষ্ঠায় দ্রষ্টব্য", "অপদ্র");
        ABBREVIATION_MAP.put("আত্মবিশ্বাস", "আবি");
        ABBREVIATION_MAP.put("আত্মীয়স্বজন", "আস্ব");
        ABBREVIATION_MAP.put("আসবাবপত্র", "আসপ");
        ABBREVIATION_MAP.put("ইতিপূর্বে", "ইপূ");
        ABBREVIATION_MAP.put("ঈষদুষ্ণ", "ঈদু");
        ABBREVIATION_MAP.put("ঈপ্সিত", "ঈত");
        ABBREVIATION_MAP.put("উচ্চাভিলাষ", "উচভি");
        ABBREVIATION_MAP.put("উদ্ভিদ বিজ্ঞান", "উবি");
        ABBREVIATION_MAP.put("ঊষাকাল", "ঊকা");
        ABBREVIATION_MAP.put("ঋণপরিশোধ", "ঋপ");
        ABBREVIATION_MAP.put("একাগ্রচিত্ত", "এচি");
        ABBREVIATION_MAP.put("একতাবদ্ধ", "একব");
        ABBREVIATION_MAP.put("কর্তব্যপরায়ণ", "কপন");
        ABBREVIATION_MAP.put("কর্মসংস্থান", "কসন");
        ABBREVIATION_MAP.put("খরচপত্র", "খচপ");
        ABBREVIATION_MAP.put("খেলাধুলা", "খেধু");
        ABBREVIATION_MAP.put("গার্হস্থ্যবিজ্ঞান", "গাবি");
        ABBREVIATION_MAP.put("গণ্ডগোল", "গগো");
        ABBREVIATION_MAP.put("গরিষ্ঠ সাধারণ গুণনীয়ক", "গসাগু");
        ABBREVIATION_MAP.put("ঘূর্ণিঝড়", "ঘৃঝ");
        ABBREVIATION_MAP.put("ঘোষণাপত্র", "ঘোপ");
        ABBREVIATION_MAP.put("চলচ্চিত্র", "চচি");
        ABBREVIATION_MAP.put("চিকিৎসা", "চিসা");
        ABBREVIATION_MAP.put("চট্টগ্রাম বিশ্ববিদ্যালয়", "চবি");
        ABBREVIATION_MAP.put("ছাত্রজীবন", "ছাজী");
        ABBREVIATION_MAP.put("ছাত্রাবাস", "ছাস");
        ABBREVIATION_MAP.put("জীববিজ্ঞান", "জীবি");
        ABBREVIATION_MAP.put("জাতিসংঘ", "জাস");
        ABBREVIATION_MAP.put("জীবনবৃত্তান্ত", "জীবৃ");
        ABBREVIATION_MAP.put("জন্মগ্রহণ", "জগ্র");
        ABBREVIATION_MAP.put("ঝড়-তুফান", "ঝতু");
        ABBREVIATION_MAP.put("টেলিভিশন", "টিভি");
        ABBREVIATION_MAP.put("টাকাপয়সা", "টাপ");
        ABBREVIATION_MAP.put("ঠাট্টা-তামাশা", "ঠাতা");
        ABBREVIATION_MAP.put("ডাকপিয়ন", "ডাপি");
        ABBREVIATION_MAP.put("ডিম্বাশয়", "ডিশয়");
        ABBREVIATION_MAP.put("ঢাকা বিশ্ববিদ্যালয়", "ঢাবি");
        ABBREVIATION_MAP.put("তড়িৎ-বিশ্লেষণ", "তবি");
        ABBREVIATION_MAP.put("তহসিলদার", "তহসি");
        ABBREVIATION_MAP.put("থার্মোমিটার", "থামি");
        ABBREVIATION_MAP.put("দৃষ্টি প্রতিবন্ধী", "দৃপ");
        ABBREVIATION_MAP.put("দীর্ঘশ্বাস", "দীস");
        ABBREVIATION_MAP.put("দূরবীক্ষণযন্ত্র", "দূবীয");
        ABBREVIATION_MAP.put("ধর্মভীরু", "ধভী");
        ABBREVIATION_MAP.put("ধর্মঘট", "ধঘ");
        ABBREVIATION_MAP.put("ধনদৌলত", "ধদৌ");
        ABBREVIATION_MAP.put("নাতিশীতোষ্ণ", "নাশীত");
        ABBREVIATION_MAP.put("নাতিদীর্ঘ", "নাদী");
        ABBREVIATION_MAP.put("নিঃসঙ্কোচ", "নিসচ");
        ABBREVIATION_MAP.put("নিত্যনৈমিত্তিক", "নিনৈ");
        ABBREVIATION_MAP.put("প্রধান শিক্ষক", "প্রশি");
        ABBREVIATION_MAP.put("প্রকৌশল বিশ্ববিদ্যালয়", "প্রবি");
        ABBREVIATION_MAP.put("প্রবর্তন", "প্রন");
        ABBREVIATION_MAP.put("পৃষ্ঠপোষক", "পৃপো");
        ABBREVIATION_MAP.put("পাপপুণ্য", "পাপু");
        ABBREVIATION_MAP.put("ফারাক্কা বাঁধ", "ফাবা");
        ABBREVIATION_MAP.put("বিমানবন্দর", "বিব");
        ABBREVIATION_MAP.put("বিশেষ দ্রষ্টব্য", "বিদ্র");
        ABBREVIATION_MAP.put("ব্যারোমিটার", "ব্যামি");
        ABBREVIATION_MAP.put("বৈদেশিক বাণিজ্য", "বৈবা");
        ABBREVIATION_MAP.put("ব্যঞ্জনবর্ণ", "ব্যব");
        ABBREVIATION_MAP.put("ভূপ্রকৃতি", "ভূপ");
        ABBREVIATION_MAP.put("ভৌগোলিক সীমারেখা", "ভৌসী");
        ABBREVIATION_MAP.put("ভাবসম্প্রসারণ", "ভাস");
        ABBREVIATION_MAP.put("মানব সমাজ", "মানজ");
        ABBREVIATION_MAP.put("মাতৃভূমি", "মাভূ");
        ABBREVIATION_MAP.put("মনোবিজ্ঞান", "মনোবি");
        ABBREVIATION_MAP.put("মহামান্য", "মমা");
        ABBREVIATION_MAP.put("মহাবিদ্যালয়", "মবি");
        ABBREVIATION_MAP.put("যৌগিক পদার্থ", "যৌপ");
        ABBREVIATION_MAP.put("রণকৌশল", "রকৌ");
        ABBREVIATION_MAP.put("রক্ষণাবেক্ষণ", "রক্ষব");
        ABBREVIATION_MAP.put("শান্তশিষ্ট", "শাশি");
        ABBREVIATION_MAP.put("শ্বাসপ্রশ্বাস", "শ্বাপ্র");
        ABBREVIATION_MAP.put("সমাজ কল্যাণ", "সক");
        ABBREVIATION_MAP.put("সমাজ সেবা", "সসে");
        ABBREVIATION_MAP.put("সমাজ বিজ্ঞান", "সবি");
        ABBREVIATION_MAP.put("সুদূর প্রসারী", "সুপ্র");
        ABBREVIATION_MAP.put("সংখ্যালঘিষ্ঠ", "সল");
        ABBREVIATION_MAP.put("সংখ্যা গরিষ্ঠ", "সগ");
        ABBREVIATION_MAP.put("সমতল ক্ষেত্র", "সক্ষে");
        ABBREVIATION_MAP.put("হস্ত শিল্প", "হশি");
        ABBREVIATION_MAP.put("হিংসা বিদ্বেষ", "হিংবি");
        ABBREVIATION_MAP.put("Bachelor of Arts", "বিএ");
        ABBREVIATION_MAP.put("Bangladesh Air Force", "বিএএফ");
        ABBREVIATION_MAP.put("United Kingdom", "ইউকে");
        ABBREVIATION_MAP.put("Horse Power", "এইচপি");
        ABBREVIATION_MAP.put("Letter of Credit", "এলসি");
        ABBREVIATION_MAP.put("বাংলাদেশ সংবাদ সংস্থা", "বাসস");
        ABBREVIATION_MAP.put("রাজধানী উন্নয়ন কর্তৃপক্ষ", "রাজউক");
        
        INTERNATIONAL_ABBREV_MAP.put("বিএ", "⠃⠁");
        INTERNATIONAL_ABBREV_MAP.put("বিএএফ", "⠃⠁⠋");
        INTERNATIONAL_ABBREV_MAP.put("ইউকে", "⠥⠅");
        INTERNATIONAL_ABBREV_MAP.put("এইচপি", "⠓⠏");
        INTERNATIONAL_ABBREV_MAP.put("এলসি", "⠇⠉");

        ABBREVIATION_MAP.put("ক্ষ", "⠟");
        ABBREVIATION_MAP.put("জ্ঞ", "⠱");
    }

    private static class ParseState {
        boolean isNumber = false;
        String quoteState = null;
    }

    public static List<List<String>> translateToBrailleCells(String text) {
        String[] lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n");
        List<List<String>> brailleCellLines = new ArrayList<>();

        for (String lineText : lines) {
            if (brailleCellLines.size() >= MAX_BRAILLE_ROWS) break;

            List<String> finalLineCells = new ArrayList<>();
            ParseState state = new ParseState();

            // Simple split by space to preserve words
            String[] words = lineText.split("(?<=\\s)|(?=\\s)");

            for (String word : words) {
                if (finalLineCells.size() >= MAX_CELLS_PER_ROW) break;
                if (word.isEmpty()) continue;

                if (word.trim().isEmpty()) {
                    finalLineCells.add(" ");
                    state.isNumber = false;
                    continue;
                }

                List<String> cells = translateWordToCells(word, state);
                for (String cellGroup : cells) {
                    if (finalLineCells.size() >= MAX_CELLS_PER_ROW) break;
                    
                    for (int i = 0; i < cellGroup.length(); i++) {
                        char c = cellGroup.charAt(i);
                        if (c >= '\u2800' && c <= '\u28FF') {
                            if (finalLineCells.size() < MAX_CELLS_PER_ROW) finalLineCells.add(String.valueOf(c));
                        } else if (c == ' ') {
                            if (finalLineCells.size() < MAX_CELLS_PER_ROW) finalLineCells.add(" ");
                        }
                    }
                }
            }

            while (finalLineCells.size() < MAX_CELLS_PER_ROW) {
                finalLineCells.add(" ");
            }
            brailleCellLines.add(finalLineCells);
        }
        return brailleCellLines;
    }

    private static List<String> translateWordToCells(String word, ParseState state) {
        List<String> cells = new ArrayList<>();
        
        // Multi-punct fast exits
        if (word.equals("----")) return new ArrayList<>(Arrays.asList("⠤", "⠤", "⠤", "⠤"));
        if (word.equals("---")) return new ArrayList<>(Arrays.asList("⠲", "⠲", "⠲"));
        if (word.equals("...")) return new ArrayList<>(Arrays.asList("⠲", "⠲", "⠲"));
        if (word.equals("--")) return new ArrayList<>(Arrays.asList("⠤", "⠤"));
        if (word.equals(":-")) return new ArrayList<>(Arrays.asList("⠒", "⠤", "⠤"));

        // Suffix handling
        String foundSuffix = null;
        String baseWord = word;
        int len = word.length();
        for (int suffixLen = 4; suffixLen >= 1; suffixLen--) {
            if (len > suffixLen) {
                String suffix = word.substring(len - suffixLen);
                if (SUFFIX_MAP.containsKey(suffix)) {
                    baseWord = word.substring(0, len - suffixLen);
                    foundSuffix = suffix;
                    break;
                }
            }
        }

        // Abbreviation handling
        if (ABBREVIATION_MAP.containsKey(baseWord)) {
            String abbrevWord = ABBREVIATION_MAP.get(baseWord);
            if (INTERNATIONAL_ABBREV_MAP.containsKey(abbrevWord)) {
                cells.add(ABBREV_PREFIX_INTERNATIONAL);
                cells.add(INTERNATIONAL_ABBREV_MAP.get(abbrevWord));
            } else {
                List<String> abbrevCells = translateWordToCells(abbrevWord, state);
                cells.addAll(abbrevCells);
            }
            if (foundSuffix != null) {
                cells.add(SUFFIX_HYPHEN);
                cells.add(SUFFIX_MAP.get(foundSuffix));
            }
            return cells;
        }

        int i = 0;
        len = word.length();

        while (i < len) {
            char c = word.charAt(i);
            Character nextC = (i + 1 < len) ? word.charAt(i + 1) : null;

            if (DIGIT_MAP.containsKey(c)) {
                if (!state.isNumber) {
                    cells.add(NUMBER_SIGN);
                }
                state.isNumber = true;
                cells.add(DIGIT_MAP.get(c));
                i++;
                continue;
            }

            state.isNumber = false;

            if (c == '"') {
                if ("double".equals(state.quoteState)) {
                    cells.add("⠴");
                    state.quoteState = null;
                } else {
                    cells.add("⠦");
                    state.quoteState = "double";
                }
                i++;
                continue;
            }

            // Juktoborno
            if (CONSONANT_MAP.containsKey(c) && nextC != null && nextC == '্') {
                List<String> conjuncts = new ArrayList<>();
                conjuncts.add(CONSONANT_MAP.get(c));
                int j = i + 2;
                while (j < len) {
                    char cur = word.charAt(j);
                    if (CONSONANT_MAP.containsKey(cur)) {
                        conjuncts.add(CONSONANT_MAP.get(cur));
                        j++;
                        if (j < len && word.charAt(j) == '্') {
                            j++;
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }
                
                int num = conjuncts.size();
                if (num >= 3) cells.add(PREFIX_JUKTO_3_PLUS);
                else if (num == 2) cells.add(PREFIX_JUKTO_2);
                
                cells.addAll(conjuncts);
                
                if (j < len && KAR_MAP.containsKey(word.charAt(j))) {
                    cells.add(KAR_MAP.get(word.charAt(j)));
                    i = j + 1;
                } else {
                    i = j;
                }
                continue;
            }

            if (CONSONANT_MAP.containsKey(c) && nextC != null && KAR_MAP.containsKey(nextC)) {
                cells.add(CONSONANT_MAP.get(c));
                cells.add(KAR_MAP.get(nextC));
                i += 2;
                continue;
            }

            if (VOWEL_MAP.containsKey(c)) cells.add(VOWEL_MAP.get(c));
            else if (CONSONANT_MAP.containsKey(c)) {
                cells.add(CONSONANT_MAP.get(c));
                if (nextC != null && VOWEL_MAP.containsKey(nextC)) {
                    cells.add(VOWEL_MAP.get('অ'));
                }
            }
            else if (OTHER_MAP.containsKey(c)) cells.add(OTHER_MAP.get(c));
            else cells.add("⠿");
            
            i++;
        }
        return cells;
    }

    private static class DotPoint implements Comparable<DotPoint> {
        int y, x, cellIndex;
        public DotPoint(int y, int x, int cellIndex) {
            this.y = y; this.x = x; this.cellIndex = cellIndex;
        }
        @Override
        public int compareTo(DotPoint o) {
            if (this.y != o.y) return Integer.compare(this.y, o.y);
            return Integer.compare(this.x, o.x);
        }
    }

    public static String translateToCommands(List<List<String>> brailleCellLines) {
        StringBuilder commands = new StringBuilder(CMD_GO_HOME);
        int current_y = 0;
        int current_x = 0;

        for (int lineIndex = 0; lineIndex < brailleCellLines.size(); lineIndex++) {
            List<String> lineCells = brailleCellLines.get(lineIndex);
            List<DotPoint> dotList = new ArrayList<>();

            for (int cellIndex = 0; cellIndex < lineCells.size(); cellIndex++) {
                String cellStr = lineCells.get(cellIndex);
                if (cellStr.equals(" ")) continue;
                
                char c = cellStr.charAt(0);
                int code = c - 0x2800;
                if (code < 0 || code > 63) continue;

                int baseX = cellIndex * STEP_COUNT_CELL_X;
                
                if ((code & 1) != 0) dotList.add(new DotPoint(0, 0, cellIndex));
                if ((code & 2) != 0) dotList.add(new DotPoint(STEP_COUNT_DOT_Y, 0, cellIndex));
                if ((code & 4) != 0) dotList.add(new DotPoint(2 * STEP_COUNT_DOT_Y, 0, cellIndex));
                if ((code & 8) != 0) dotList.add(new DotPoint(0, STEP_COUNT_DOT_X, cellIndex));
                if ((code & 16) != 0) dotList.add(new DotPoint(STEP_COUNT_DOT_Y, STEP_COUNT_DOT_X, cellIndex));
                if ((code & 32) != 0) dotList.add(new DotPoint(2 * STEP_COUNT_DOT_Y, STEP_COUNT_DOT_X, cellIndex));
            }

            Collections.sort(dotList);

            for (DotPoint dp : dotList) {
                int target_x = dp.cellIndex * STEP_COUNT_CELL_X + dp.x;
                int target_y = dp.y;

                if (target_y != current_y) {
                    int y_moves = (target_y - current_y) / STEP_COUNT_DOT_Y;
                    if (y_moves > 0) {
                        for(int i=0; i<y_moves; i++) commands.append(CMD_Y_SHIFT_DOT_ROW);
                    } else if (y_moves < 0) {
                        commands.append(CMD_GO_HOME);
                        current_x = 0; current_y = 0;
                        y_moves = target_y / STEP_COUNT_DOT_Y;
                        for(int i=0; i<y_moves; i++) commands.append(CMD_Y_SHIFT_DOT_ROW);
                    }
                    current_y = target_y;
                }

                int x_diff = target_x - current_x;
                if (x_diff > 0) {
                    int moves = x_diff / STEP_COUNT_DOT_X; // Using standard units for simplicity
                    for(int i=0; i<x_diff; i++) commands.append(CMD_DOT_SHIFT_X); // Keep consistent with python logic
                } else if (x_diff < 0) {
                    commands.append(CMD_GO_HOME).append(CMD_GO_HOME);
                    current_x = 0; current_y = 0;
                    for(int i=0; i<target_x; i++) commands.append(CMD_DOT_SHIFT_X);
                    int y_moves = target_y / STEP_COUNT_DOT_Y;
                    for(int i=0; i<y_moves; i++) commands.append(CMD_Y_SHIFT_DOT_ROW);
                }
                current_x = target_x;
                commands.append(CMD_PRINT);
            }

            if (lineIndex < brailleCellLines.size() - 1) {
                commands.append(CMD_LINE_FEED);
                commands.append(CMD_GO_HOME);
                current_x = 0; current_y = 0;
            }
        }
        
        String res = commands.toString();
        while(res.endsWith(CMD_GO_HOME)) res = res.substring(0, res.length() - 1);
        return res;
    }
}
