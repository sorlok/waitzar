/*
 * Copyright 2010 by Seth N. Hetu
 *
 * Please refer to the end of the file for licensing information
 */

#include "RomanInputMethod.h"


using namespace waitzar;
using std::vector;
using std::pair;
using std::string;
using std::wstring;


//This takes responsibility for the model and sentence memory.

void RomanInputMethod::init(MyWin32Window* mainWindow, MyWin32Window* sentenceWindow, MyWin32Window* helpWindow,MyWin32Window* memoryWindow, const std::vector< std::pair <int, unsigned short> > &systemWordLookup, OnscreenKeyboard *helpKeyboard, std::wstring systemDefinedWords, LookupEngine* model, waitzar::SentenceList* sentence, const std::wstring& encoding, CONTROL_KEY_TYPE controlKeyStyle, bool typeBurmeseNumbers, bool typeNumeralConglomerates, bool suppressUppercase)
{
	InputMethod::init(mainWindow, sentenceWindow, helpWindow, memoryWindow, systemWordLookup, helpKeyboard, systemDefinedWords, encoding, controlKeyStyle, typeBurmeseNumbers, typeNumeralConglomerates, suppressUppercase);

	this->model = model;
	this->sentence = sentence;

	//Save properties
	this->controlKeyStyle = controlKeyStyle;
	this->typeBurmeseNumbers = typeBurmeseNumbers;
	this->typeNumeralConglomerates = typeNumeralConglomerates;
	this->suppressUppercase = suppressUppercase;
}




std::pair<int, std::string> RomanInputMethod::lookupWord(std::wstring typedWord)
{
	return model->reverseLookupWord(typedWord);
}



bool RomanInputMethod::handleVKey(VirtKey& vkey)
{
	//Override a few dispatches
	if (vkey.alphanumByLocale==',' || vkey.alphanumByLocale=='.') {
		//Handle stops
		this->handleStop(vkey.alphanumByLocale=='.', vkey);
		return true;
	} else if (vkey.alphanumByLocale>='0' && vkey.alphanumByLocale<='9') {
		//Handle numbers and combiners; pick a word, type a letter, combine/stack letters.
		//Handle key press; letter-based keyboard should just pass this on through
		this->handleNumber(vkey);
		return true;
	} else {
		//Default: Let InputMethod handle it.
		return InputMethod::handleVKey(vkey);
	}
	return false;
}




void RomanInputMethod::handleEsc()
{
	//Escape out of the main window or the sentence, depending
	if (!mainWindow->isVisible()) {
		//Kill the entire sentence
		sentence->clear();
	} else {
		//Cancel the current word
		typedRomanStr.str(L"");
		viewChanged = true; //Shouldn't matter, but doesn't hurt.
	}
}




void RomanInputMethod::handleBackspace(VirtKey& vkey)
{
	if (!mainWindow->isVisible()) {
		//Delete the previous word in the sentence
		if (sentence->deletePrev(*model))
			viewChanged = true;
	} else {
		//Delete the previously-typed letter
		model->backspace(sentence->getPrevTypedWord(*model, userDefinedWords));

		//Truncate...
		wstring newStr = !typedRomanStr.str().empty() ? typedRomanStr.str().substr(0, typedRomanStr.str().length()-1) : L"";
		typedRomanStr.str(L"");
		typedRomanStr <<newStr;
		viewChanged = true;
	}
}



void RomanInputMethod::handleDelete()
{
	if (!mainWindow->isVisible()) {
		//Delete the next word
		if (sentence->deleteNext())
			viewChanged = true;
	}
}



void RomanInputMethod::handleLeftRight(bool isRight, bool loopToZero)
{
	int amt = isRight ? 1 : -1;
	if (mainWindow->isVisible()) {
		//Move right/left within the current selection.
		if (model->moveRight(amt) == TRUE)
			viewChanged = true;
		else if (isRight && loopToZero) {
			//Force back to index zero
			model->moveRight(-1000); //Hacky, we should eventually set a better wraparound method.
			viewChanged = true;
		}
	} else {
		//Move right/left within the current phrase.
		if (sentence->moveCursorRight(amt, *model))
			viewChanged = true;
	}
}



void RomanInputMethod::handleTab()
{
	if (mainWindow->isVisible()) {
		//Change the selection, or make a selection (depending on the style)
		if (controlKeyStyle==CONTROL_KEY_TYPE::CHINESE)
			handleLeftRight(true, true);
		else if (controlKeyStyle==CONTROL_KEY_TYPE::JAPANESE)
			handleCommit(true);
	} else {
		//Move the sentence window cursor
		handleLeftRight(true, false);
	}

}



void RomanInputMethod::handleUpDown(bool isDown)
{
	if (mainWindow->isVisible()) {
		if (model->pageUp(!isDown))
			viewChanged = true;
	}
}



void RomanInputMethod::handleNumber(VirtKey& vkey)
{
	//Special case: conglomerate numbers
	if ((vkey.alphanum()>='0'&&vkey.alphanum()<='9') && typeNumeralConglomerates && typeBurmeseNumbers && typedStrContainsNoAlpha) {
	 if (model->typeLetter(vkey.alphanum(), vkey.modShift, sentence->getPrevTypedWord(*model, userDefinedWords))) {
		 typedRomanStr <<vkey.alphanum();
		 viewChanged = true;
	 }
	} else if (mainWindow->isVisible()) {
		//Convert 1..0 to 0..9
		int numMinOne = vkey.alphanum() - '0' - 1;
		if (numMinOne<0)
			numMinOne = 9;

		//A bit of mangling for pages
		numMinOne += model->getCurrPage()*10;

		//Mangle a bit more as usual...
		//if (wParam==HOTKEY_COMBINE || wParam==HOTKEY_SHIFT_COMBINE)
		if (vkey.alphanum()=='`' || vkey.alphanum()=='~') //TODO: Find a better way to do this later. (Not crucial.)
			numMinOne = -1;

		//Select this numbered word
		if (selectWord(numMinOne)) {
			typedRomanStr.str(L"");
			viewChanged = true;
		}
	} else if (vkey.alphanum()=='`' || vkey.alphanum()=='~') {
		//Check for system keys
		InputMethod::handleKeyPress(vkey);
	} else {
		//Main window is not visible and we are typing 0-9. But what about BurmeseNumerals?
		if (typeBurmeseNumbers) {
			//Type this number --ask the model for the number directly, to avoid crashing Burglish.
			sentence->insert(model->getSingleDigitID(vkey.alphanum() - '0'));
			sentence->moveCursorRight(0, true, *model);
		} else /*if (sentenceWindow->isVisible())*/ {
			//As long as the numbers 0-9 are in the "system key" list (they are) then we can just pass this off.
			InputMethod::handleKeyPress(vkey);
		}

		viewChanged = true;
	}
}



void RomanInputMethod::handleStop(bool isFull, VirtKey& vkey)
{
	unsigned short stopChar = model->getStopCharacter(isFull);
	if (!mainWindow->isVisible()) {
		//Otherwise, we perform the normal "enter" routine.
		typedStopChar = (wchar_t)stopChar;
		requestToTypeSentence = true;
	}
}




void RomanInputMethod::handleCommit(bool strongCommit)
{
	if (mainWindow->isVisible()) {
		//The model is visible; react to the control key style.
		if (!strongCommit && controlKeyStyle==CONTROL_KEY_TYPE::JAPANESE) {
			//Advance
			handleLeftRight(true, true);
		} else {
			//Select the current word
			if (selectCurrWord()) {
				//Reset, recalc
				typedRomanStr.str(L"");
				viewChanged = true;
			}
		}
	} else {
		//A bit tricky here. If the cursor's at the end, we'll
		//  do HOTKEY_ENTER. But if not, we'll just advance the cursor.
		//Hopefully this won't confuse users so much.
		//Note: ENTER overrides this behavior.
		if (strongCommit) {
			//Type the entire sentence
			requestToTypeSentence = true;
		} else {
			if (sentence->getCursorIndex()==-1 || sentence->getCursorIndex()<((int)sentence->size()-1)) {
				sentence->moveCursorRight(1, *model);
				viewChanged = true;
			} else {
				//Type the entire sentence
				requestToTypeSentence = true;
			}
		}
	}
}





void RomanInputMethod::handleKeyPress(VirtKey& vkey)
{
	//Handle regular letter-presses (as lowercase)
	//NOTE: ONLY handle letters
	wchar_t alpha = vkey.alphanum();
	if ((alpha>='a' && alpha<='z') || alpha==';') {
		//Run this keypress into the model. Accomplish anything?
		if (!model->typeLetter(vkey.alphanum(), suppressUppercase?false:vkey.modShift, sentence->getPrevTypedWord(*model, userDefinedWords))) {
			//That's the end of the story if we're typing Chinese-style; or if there's no roman string.
			if (controlKeyStyle==CONTROL_KEY_TYPE::CHINESE || typedRomanStr.str().empty())
				return;

			//Otherwise, try typing the current string directly
			handleCommit(true);
			viewChanged = true;

			//Nothing left on the new string?
			if (!model->typeLetter(vkey.alphanum(), suppressUppercase?false:vkey.modShift, sentence->getPrevTypedWord(*model, userDefinedWords)))
				return;
		}

		//Update the alhpanum property
		if (!suppressUppercase && vkey.modShift && alpha>='a' && alpha<='z')
			alpha = (alpha-'a') + 'A';

		//Update the romanized string, trigger repaint
		typedStrContainsNoAlpha = false;
		typedRomanStr <<alpha; //alphanum will always be something visible, or '\0' if nothing on the keyboard.
		viewChanged = true;
	} else {
		//Check for system keys
		InputMethod::handleKeyPress(vkey);
	}
}




bool RomanInputMethod::selectCurrWord()
{
	return this->selectWord(model->getCurrSelectedID());
}




//NOTE: The only time a negative ID is used is if we're choosing a PS combination.

bool RomanInputMethod::selectWord(int id)
{
	//Are there any words to use?
	std::pair<int, int> typedVal = model->typeSpace(id);
	if (typedVal.first<0)
		return false;
	int wordID = typedVal.first;

	//Pat-sint clears the previous word, changes what we're inserting
	if (typedVal.second>=0) {
		sentence->deletePrev(*model);
		wordID = typedVal.second;
	}

	//Insert into the current sentence, return
	sentence->insert(wordID);
	return true;
}



void RomanInputMethod::typeHelpWord(std::string roman, std::wstring myanmar, int currStrDictID)
{
	//Add it to the memory list
	mostRecentRomanizationCheck.first = roman;
	mostRecentRomanizationCheck.second = myanmar;

	//Add it to the dictionary?
	if (currStrDictID==-1) {
		//wstring tempStr = waitzar::sortMyanmarString(myanmar); //We've already passed in the native encoding
		userDefinedWords.push_back(myanmar);
		currStrDictID = -1*(systemDefinedWords.size()+userDefinedWords.size());
	}

	//Type this word (should always succeed)
	this->appendToSentence('\0', currStrDictID);

	//Update trigrams
	sentence->updateTrigrams(*model);
}




vector<wstring> RomanInputMethod::getTypedSentenceStrings()
{
	//Results
	//TODO: Cache the results.
	vector<wstring> res;
	std::wstringstream line;
	std::wstringstream full;

	//Build
	int currID = -1;
	int actID = model->getCurrSelectedID()+model->getFirstWordIndex();
	int currReplacementID = (model->getPossibleWords().empty()) ? -1 : model->getWordCombinations()[actID];
	for (std::list<int>::const_iterator it=sentence->begin(); it!=sentence->end(); it++) {
		//Get the word. This depends on the ID's negativity, size, etc.
		wstring currWord;
		if (*it>=0)
			currWord = model->getWordString(*it);
		else {
			int modID = -(*it)-1;
			if (modID<(int)systemDefinedWords.size())
				currWord = wstring(1, systemDefinedWords[modID]);
			else
				currWord = userDefinedWords[modID-systemDefinedWords.size()];
		}

		//Have we reached a transition?
		if (currID==sentence->getCursorIndex()-1 && currReplacementID!=-1) {
			//We're about to start the highlighted word.
			res.push_back(line.str());
			line.str(L"");

			//CHANGE the current word to the pat-sint replacement.
			currWord = model->getWordString(currReplacementID);
		} else if (currID==sentence->getCursorIndex()) {
			//We're at the cursor
			res.push_back(line.str());
			line.str(L"");
			if (res.size()==1)
				res.push_back(L"");
		}

		//Append the word
		line <<currWord;
		full <<currWord;

		//Increment
		currID++;
	}

	//Add the final entry, and the typedStopChar if necessary
	if (typedStopChar!=L'\0') {
		line <<typedStopChar;
		full <<typedStopChar;
	}
	res.push_back(line.str());

	//I think this might always be true...
	//TODO: Check the source and remove these tests.
	while (res.size()<3)
		res.push_back(L"");
	if (res.size()!=3)
		throw std::runtime_error("Error! getTypedSentenceStr() must always return a vector of size 3+1");

	//Finally, add the full entry
	res.push_back(full.str());

	//Done
	return res;
}


//0 = reg. word
//1 = pat-sint
//2 = selected
//4 = give it a "tilde" label

vector< pair<wstring, unsigned int> > RomanInputMethod::getTypedCandidateStrings()
{
	//TODO: cache the results
	std::vector< std::pair<std::wstring, unsigned int> > res;
	std::vector<unsigned int> words = model->getPossibleWords();
	std::vector<int> combinations = model->getWordCombinations();
	for (size_t i=0; i<words.size(); i++) {
		std::pair<std::wstring, unsigned int> item = std::pair<std::wstring, unsigned int>(model->getWordString(words[i]), HF_NOTHING);

		//Get the previous word
		std::wstring prevWord = sentence->getPrevTypedWord(*model, userDefinedWords);

		//Color properly.
		if (combinations[i]!=-1) {
			item.second |= HF_PATSINT;
			if (model->canTypeShortcut())
				item.second |= HF_LABELTILDE;
		}
		if (model->getCurrSelectedID() == (int)i-(int)model->getFirstWordIndex())
			item.second |= HF_CURRSELECTION;
		res.push_back(item);
	}

	return res;
}



void RomanInputMethod::appendToSentence(wchar_t letter, int id)
{
	//Type it
	//selectWord(id);
	sentence->insert(id);

	//We need to reset the trigrams here...
	sentence->updateTrigrams(*model);

	//Repaint
	viewChanged = true;
}



void RomanInputMethod::reset(bool resetCandidates, bool resetRoman, bool resetSentence, bool performFullReset)
{
	//A "full" reset entails the others
	if (performFullReset) {
		resetCandidates = resetRoman = resetSentence = true;
		userDefinedWords.clear();
	}

	//Equivalent to reset candidates or the roman string
	if (resetCandidates || resetRoman)  {
		typedRomanStr.str(L"");
		model->reset(performFullReset);
	}

	//Reset the sentence?
	if (resetSentence)
		sentence->clear();

	if (resetRoman)
		typedStrContainsNoAlpha = true;

	//Either way
	typedStopChar = L'\0';
}

//Add our paren string

std::wstring RomanInputMethod::getTypedRomanString(bool asLowercase)
{
	//Initail string
	std::wstringstream res;
	res <<InputMethod::getTypedRomanString(asLowercase);

	//Add a paren string, if it exists
	if (!model->getParenString().empty())
		res <<L'(' <<model->getParenString() <<L')';

	//Done
	return res.str();
}


std::pair<int, int> RomanInputMethod::getPagingInfo() const
{
	return std::pair<int, int>(model->getCurrPage(), model->getNumberOfPages());
}



void RomanInputMethod::treatAsHelpKeyboard(InputMethod* providingHelpFor, std::function<void (const std::wstring& fromEnc, const std::wstring& toEnc, std::wstring& src)> ConfigGetAndTransformSrc)
{
	throw std::runtime_error("Cannot use a Romanized keyboard for a help input method");
}



/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
