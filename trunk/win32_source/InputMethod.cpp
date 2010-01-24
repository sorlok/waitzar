/*
 * Copyright 2010 by Seth N. Hetu
 *
 * Please refer to the end of the file for licensing information
 */

#include "InputMethod.h"

InputMethod::InputMethod(MyWin32Window* mainWindow, MyWin32Window* sentenceWindow, MyWin32Window* helpWindow, MyWin32Window* memoryWindow, const vector< pair <int, unsigned short> > &systemWordLookup)
{
	//Init
	isHelpInput = false;

	//Save
	this->mainWindow = mainWindow;
	this->sentenceWindow = sentenceWindow;
	this->helpWindow = helpWindow;
	this->memoryWindow = memoryWindow;
	this->systemWordLookup = systemWordLookup;
}

InputMethod::~InputMethod()
{
}


void InputMethod::treatAsHelpKeyboard(bool val)
{
	this->isHelpInput = val;
}



//Handle system keys
void InputMethod::handleKeyPress(WPARAM wParam)
{
	//Get an adjusted numcode.
	int base = (wParam>=HOTKEY_0 && wParam<=HOTKEY_9) ? HOTKEY_0 : (wParam>=HOTKEY_NUM0 && wParam<=HOTKEY_NUM9) ? HOTKEY_NUM0 : -1;
	int numCode = (base==-1) ? wParam : HOTKEY_0 + (int)wParam - base;

	//Check system keys, but ONLY if the sentence window is the only thing visible.
	if (!helpWindow->isVisible() && !mainWindow->isVisible()) {
		wchar_t letter = '\0';
		for (size_t i=0; i<systemWordLookup.size(); i++) {
			if (systemWordLookup[i].first==numCode) {
				//This represents a negative offset
				numCode = -1-i;
				letter = systemWordLookup[i].secong;
				break;
			}

			//Didn't find it?
			if (i==systemWordLookup.size()-1)
				return;
		}

		//Try to type this word; we now have its numCode and letter
		this->appendToSentence(letter, numCode);
	}
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
