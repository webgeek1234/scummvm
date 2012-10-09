/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "neverhood/module1600.h"
#include "neverhood/gamemodule.h"
#include "neverhood/module1200.h"
#include "neverhood/module2200.h"

namespace Neverhood {

static const uint32 kModule1600SoundList[] = {
	0x90805C50,
	0x90804450,
	0xB4005E60,
	0x91835066,
	0x90E14440,
	0
};

Module1600::Module1600(NeverhoodEngine *vm, Module *parentModule, int which)
	: Module(vm, parentModule) {
	
	if (which < 0) {
		createScene(_vm->gameState().sceneNum, -1);
	} else if (which == 1) {
		createScene(4, 1);
	} else if (which == 2) {
		createScene(5, 0);
	} else if (which == 3) {
		createScene(6, 1);
	} else if (which == 4) {
		createScene(1, 0);
	} else {
		createScene(0, 0);
	}

	_vm->_soundMan->addSoundList(0x1A008D8, kModule1600SoundList);
	_vm->_soundMan->setSoundListParams(kModule1600SoundList, true, 50, 600, 5, 150);
	_vm->_soundMan->playTwoSounds(0x1A008D8, 0x41861371, 0x43A2507F, 0);

}

Module1600::~Module1600() {
	_vm->_soundMan->deleteGroup(0x1A008D8);
}

void Module1600::createScene(int sceneNum, int which) {
	debug("Module1600::createScene(%d, %d)", sceneNum, which);
	_vm->gameState().sceneNum = sceneNum;
	switch (_vm->gameState().sceneNum) {
	case 0:
		createNavigationScene(0x004B39D0, which);
		break;
	case 1:
		createNavigationScene(0x004B3A30, which);
		break;
	case 2:
		createNavigationScene(0x004B3A60, which);
		break;
	case 3:
		createNavigationScene(0x004B3A90, which);
		break;
	case 4:
		createNavigationScene(0x004B3B20, which);
		break;
	case 5:
		createNavigationScene(0x004B3B50, which);
		break;
	case 6:
		createNavigationScene(0x004B3B80, which);
		break;
	case 7:
		_childObject = new Scene1608(_vm, this, which);
		break;
	case 8:
		_childObject = new Scene1609(_vm, this, which);
		break;
	case 1001:
		if (getGlobalVar(0xA0808898) == 1) {
			createSmackerScene(0x80050200, true, true, false);
		} else if (getGlobalVar(0xA0808898) == 2) {
			createSmackerScene(0x80090200, true, true, false);
		} else {
			createSmackerScene(0x80000200, true, true, false);
		}
		if (getGlobalVar(0xA0808898) >= 2)
			setGlobalVar(0xA0808898, 0);
		else
			incGlobalVar(0xA0808898, +1);			
		break;
	}
	SetUpdateHandler(&Module1600::updateScene);
	_childObject->handleUpdate();
}

void Module1600::updateScene() {
	if (!updateChild()) {
		switch (_vm->gameState().sceneNum) {
		case 0:
			if (_moduleResult == 0)
				createScene(2, 0);
			else if (_moduleResult == 1)
				createScene(1, 0);
			else if (_moduleResult == 2)
				leaveModule(4);
			break;
		case 1:
			if (_moduleResult == 0)
				createScene(1001, -1);
			else if (_moduleResult == 1)
				createScene(0, 3);
			break;
		case 2:
			if (_moduleResult == 0)
				createScene(3, 0);
			else if (_moduleResult == 1)
				createScene(0, 2);
			break;
		case 3:
			if (_moduleResult == 0)
				createScene(5, 0);
			else if (_moduleResult == 2)
				createScene(6, 0);
			else if (_moduleResult == 3)
				createScene(2, 1);
			else if (_moduleResult == 4)
				createScene(4, 0);
			break;
		case 4:
			if (_moduleResult == 0)
				leaveModule(1);
			else if (_moduleResult == 1)
				createScene(3, 1);
			break;
		case 5:
			if (_moduleResult == 0)
				leaveModule(2);
			else if (_moduleResult == 1)
				createScene(3, 3);
			break;
		case 6:
			if (_moduleResult == 0)
				createScene(8, -1);
			else if (_moduleResult == 1)
				createScene(3, 5);
			break;
		case 7:
			createScene(6, 1);
			break;
		case 8:
			if (_moduleResult == 0)
				createScene(6, 0);
			else
				createScene(7, 0);
			break;
		case 1001:
			createScene(1, 0);
			break;
		}
	}
}

AsCommonCar::AsCommonCar(NeverhoodEngine *vm, Scene *parentScene, int16 x, int16 y)
	: AnimatedSprite(vm, 1000), _parentScene(parentScene) {
	
	SetUpdateHandler(&AsCommonCar::update);
	SetMessageHandler(&AsCommonCar::handleMessage);
	SetSpriteUpdate(NULL);

	// TODO createSurface2(200, dword_4AF4C0);
	createSurface(200, 640, 480); //TODO: Remove once the line above is done
	_x = x;
	_y = y;
	
	_field100 = 0;
	_exitDirection = 0;
	_currPointIndex = 0;
	_againDestPtFlag = 0;
	_stepError = 0;
	_againDestPointFlag = 0;
	_steps = 0;
	_flag10E = 0;
	_moreY = 0;
	_flag10F = 0;
	_flag113 = 0;
	_flag114 = 1;
	_flag11A = 0;
	_newDeltaXType = -1;
	_field11E = 0;
	_pathPoints = NULL;
	_rectList = NULL;
	
	startAnimation(0xD4220027, 0, -1);
	setDoDeltaX(getGlobalVar(0x21E60190));

}

AsCommonCar::~AsCommonCar() {
	if (_finalizeStateCb == AnimationCallback(&AsCommonCar::sub45D620)) {
		setGlobalVar(0x21E60190, !getGlobalVar(0x21E60190));
	}
}

void AsCommonCar::setPathPoints(NPointArray *pathPoints) {
	_pathPoints = pathPoints;
}

void AsCommonCar::update() {
	if (_newDeltaXType >= 0) {
		setDoDeltaX(_newDeltaXType);
		_newDeltaXType = -1;
	}
	AnimatedSprite::update();
	if (_againDestPtFlag && _moreY == 0 && !_flag10F) {
		_againDestPtFlag = 0;
		_againDestPointFlag = 0;
		sendPointMessage(this, 0x2004, _againDestPt);
	} else if (_againDestPointFlag && _moreY == 0 && !_flag10F) {
		_againDestPointFlag = 0;
		sendMessage(this, 0x2003, _againDestPointIndex);
	}
	sub45CE10();
	sub45E0A0();
}

void AsCommonCar::update45C790() {
	AsCommonCar::update();
	if (++_idleCounter >= _idleCounterMax)
		sub45D050();
	sub45E0A0();
}

uint32 AsCommonCar::handleMessage(int messageNum, const MessageParam &param, Entity *sender) {
	uint32 messageResult = Sprite::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x1019:
		SetSpriteUpdate(NULL);
		break;
	/* NOTE: Implemented in setPathPoints
	case 0x2000:
	case 0x2001:
	*/
	case 0x2002:
		// Set the current position without moving
		_currPointIndex = param.asInteger();
		_stepError = 0;
		_x = pathPoint(_currPointIndex).x;
		_y = pathPoint(_currPointIndex).y;
		break;
	case 0x2003:
		// Move to a point by its index
		{
			int newPointIndex = param.asInteger();
			if (_moreY <= 0 && !_flag10F) {
				_someX = pathPoint(newPointIndex).x;
				_someY = pathPoint(newPointIndex).y;
				if (_currPointIndex < newPointIndex) {
					moveToNextPoint();
				} else if (_currPointIndex == newPointIndex && _stepError == 0) {
					if (_currPointIndex == 0) {
						_moreY = 0;
						sendMessage(_parentScene, 0x2005, 0);
					} else if (_currPointIndex == (int)_pathPoints->size()) {
						_moreY = 0;
						sendMessage(_parentScene, 0x2006, 0);
					}
				} else {
					moveToPrevPoint();
				}
			} else {
				_againDestPointFlag = 1;
				_againDestPointIndex = newPointIndex;
			}
		}
		break;
	case 0x2004:
		// Move to the point closest to the parameter point
		{
			int minMatchIndex = -1;
			int minMatchDistance, distance;
			NPoint pt = param.asPoint();
			if (_moreY <= 0 && !_flag10F) {
				// Check if we're already exiting (or something)
				if ((pt.x <= 20 && _exitDirection == 1) ||
					(pt.x >= 620 && _exitDirection == 3) ||
					(pt.y <= 20 && _exitDirection == 2) ||
					(pt.y >= 460 && _exitDirection == 4))
					break;
				_someX = pt.x;
				_someY = pt.y;
				minMatchDistance = calcDistance(_someX, _someY, _x, _y) + 1;
				for (int i = _currPointIndex + 1; i < (int)_pathPoints->size(); i++) {
					distance = calcDistance(_someX, _someY, pathPoint(i).x, pathPoint(i).y);
					if (distance >= minMatchDistance)
						break;
					minMatchDistance = distance;
					minMatchIndex = i;
				}
				for (int i = _currPointIndex; i >= 0; i--) {
					distance = calcDistance(_someX, _someY, pathPoint(i).x, pathPoint(i).y);
					if (distance >= minMatchDistance)
						break;
					minMatchDistance = distance;
					minMatchIndex = i;
				}
				if (minMatchIndex == -1) {
					if (_currPointIndex == 0) {
						moveToPrevPoint();
					} else {
						SetSpriteUpdate(NULL);
					}
				} else {
					if (minMatchIndex > _currPointIndex) {
						moveToNextPoint();
					} else {
						moveToPrevPoint();
					}
				}
			} else {
				_againDestPtFlag = 1;
				_againDestPt = pt;
			}
		}
		break;
	case 0x2007:
		_moreY = param.asInteger();
		_steps = 0;
		_flag10E = 0;
		SetSpriteUpdate(&AsCommonCar::suMoveToPrevPoint);
		_lastDistance = 640;
		break;
	case 0x2008:
		_moreY = param.asInteger();
		_steps = 0;
		_flag10E = 0;
		SetSpriteUpdate(&AsCommonCar::suMoveToNextPoint);
		_lastDistance = 640;
		break;
	case 0x2009:
		sub45CF80();
		break;
	case 0x200A:
		sub45CFB0();
		break;
	/* NOTE: Implemented in setRectList
	case 0x200B:
	case 0x200C:
	*/
	case 0x200E:
		sub45D180();
		break;
	case 0x200F:
		sub45CD00();
		_newDeltaXType = param.asInteger();
		break;
	}
	return messageResult;
}

uint32 AsCommonCar::handleMessage45CC30(int messageNum, const MessageParam &param, Entity *sender) {
	uint32 messageResult = AsCommonCar::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x100D:
		if (_flag10F && param.asInteger() == 0x025424A2) {
			gotoNextState();
		}
		break;
	case 0x3002:
		gotoNextState();
		break;
	}
	return messageResult;
}

uint32 AsCommonCar::handleMessage45CCA0(int messageNum, const MessageParam &param, Entity *sender) {
	switch (messageNum) {
	case 0x2009:
		sub45CF80();
		break;
	case 0x3002:
		sendMessage(_parentScene, 0x200A, 0);
		SetMessageHandler(&AsCommonCar::handleMessage);
		break;
	}
	return 0;
}

void AsCommonCar::sub45CD00() {
	bool doDeltaX = _doDeltaX;
	SetSpriteUpdate(NULL);
	_againDestPtFlag = 0;
	_againDestPointFlag = 0;
	_flag10E = 0;
	_flag10F = 0;
	_flag113 = 0;
	_flag114 = 0;
	_flag11A = 0;
	_rectList = NULL;
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	NextState(&AsCommonCar::sub45CFE0);
	startAnimation(0x35698F78, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update45C790);
	FinalizeState(&AsCommonCar::sub45D040);
	setDoDeltaX(doDeltaX ? 1 : 0);
	_currMoveDirection = 0;
	_newMoveDirection = 0;
	_steps = 0;
	_idleCounter = 0;
	_idleCounterMax = _vm->_rnd->getRandomNumber(64 - 1) + 24;
}

void AsCommonCar::sub45CDC0() {
	if (_value112 == 1) {
		_lastDistance = 640;
		_flag113 = 0;
		_flag10E = 0;
		SetSpriteUpdate(&AsCommonCar::suMoveToNextPoint);
	} else if (_value112 == 2) {
		_lastDistance = 640;
		_flag113 = 0;
		_flag10E = 0;
		SetSpriteUpdate(&AsCommonCar::suMoveToPrevPoint);
	}
}

void AsCommonCar::sub45CE10() {
	if (_flag10E && !_flag113 && !_flag10F) {
		gotoNextState();
		_flag114 = 0;
		_flag113 = 1;
		startAnimation(0x192ADD30, 0, -1);
		SetMessageHandler(&AsCommonCar::handleMessage45CC30);
		SetUpdateHandler(&AsCommonCar::update);
		NextState(&AsCommonCar::sub45CFE0);
	} else if (!_flag10E && _steps && _flag113) {
		gotoNextState();
		_flag113 = 0;
		startAnimation(0x9966B138, 0, -1);
		SetMessageHandler(&AsCommonCar::handleMessage45CC30);
		SetUpdateHandler(&AsCommonCar::update);
		NextState(&AsCommonCar::sub45D100);
	} else {
		bool flag = false;
		uint index = 0;
		if (_rectList && _rectList->size() > 0) {
			while (index < _rectList->size()) {
				NRect r = (*_rectList)[index];
				if (_x >= r.x1 && _x <= r.x2 && _y >= r.y1 && _y <= r.y2)
					break;
			}
			if (index < _rectList->size() && !_flag11A)
				flag = true;
			_flag11A = index < _rectList->size();
		}
		if (flag) {
			gotoNextState();
			sub45D0A0();
		} else if (_newMoveDirection != _currMoveDirection && _flag114 && !_flag10F) {
			gotoNextState();
			_currMoveDirection = _newMoveDirection;
			sub45D100();
		}
	}
}

void AsCommonCar::sub45CF80() {
	startAnimation(0xA86A9538, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	NextState(&AsCommonCar::sub45CFE0);
}

void AsCommonCar::sub45CFB0() {
	startAnimation(0xA86A9538, -1, -1);
	_playBackwards = true;
	SetMessageHandler(&AsCommonCar::handleMessage45CCA0);
	SetUpdateHandler(&AsCommonCar::update);
}

void AsCommonCar::sub45CFE0() {
	startAnimation(0x35698F78, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage);
	SetUpdateHandler(&AsCommonCar::update45C790);
	FinalizeState(&AsCommonCar::sub45D040);
	_idleCounter = 0;
	_currMoveDirection = 0;
	_newMoveDirection = 0;
	_steps = 0;
	_idleCounterMax = _vm->_rnd->getRandomNumber(64 - 1) + 24;
}

void AsCommonCar::sub45D040() {
	SetUpdateHandler(&AsCommonCar::update);
}

void AsCommonCar::sub45D050() {
	startAnimation(0xB579A77C, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	NextState(&AsCommonCar::sub45CFE0);
	_idleCounter = 0;
	_idleCounterMax = _vm->_rnd->getRandomNumber(64 - 1) + 24;
}

void AsCommonCar::sub45D0A0() {
	_flag10F = 1;
	gotoNextState();
	startAnimation(0x9C220DA4, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	FinalizeState(&AsCommonCar::sub45D0E0);
}

void AsCommonCar::sub45D0E0() {
	_flag10F = 0;
	_newMoveDirection = 0;
	sub45D100();
}

void AsCommonCar::sub45D100() {
	_flag114 = 1;
	if (_currMoveDirection == 1) {
		startAnimation(0xD4AA03A4, 0, -1);
	} else if (_currMoveDirection == 3) {
		startAnimation(0xD00A1364, 0, -1);
	} else if ((_currMoveDirection == 2 && _doDeltaX) || (_currMoveDirection == 4 && !_doDeltaX)) {
		sub45D180();
	} else {
		startAnimation(0xD4220027, 0, -1);
	}
	setGlobalVar(0x21E60190, _doDeltaX ? 1 : 0);
}

void AsCommonCar::sub45D180() {
	_flag10F = 1;
	gotoNextState();
	startAnimation(0xF46A0324, 0, -1);
	_value112 = 0;
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	FinalizeState(&AsCommonCar::sub45D620);
	sub45CDC0();
}

void AsCommonCar::moveToNextPoint() {
	if (_currPointIndex >= (int)_pathPoints->size() - 1) {
		_moreY = 0;
		sendMessage(this, 0x1019, 0);
		sendMessage(_parentScene, 0x2006, 0);
	} else {
		NPoint nextPt = pathPoint(_currPointIndex + 1);
		NPoint currPt = pathPoint(_currPointIndex);
		if (ABS(nextPt.y - currPt.y) <= ABS(nextPt.x - currPt.x) && nextPt.x >= currPt.x &&
			(_currMoveDirection == 4 || _currMoveDirection == 2)) {
			if (_currMoveDirection == 4) 
				_currMoveDirection = 2;
			else if (_currMoveDirection == 2)
				_currMoveDirection = 4;
			if (_flag113)
				sub45D390();
			else
				sub45D350();
		} else {
			if (_steps == 0) {
				gotoNextState();
				_flag113 = 0;
				startAnimation(0x9966B138, 0, -1);
				SetMessageHandler(&AsCommonCar::handleMessage45CC30);
				SetUpdateHandler(&AsCommonCar::update);
				NextState(&AsCommonCar::sub45D100);
			}
			_flag10E = 0;
			SetSpriteUpdate(&AsCommonCar::suMoveToNextPoint);
			_lastDistance = 640;
		}
	}
}

void AsCommonCar::sub45D350() {
	gotoNextState();
	_flag10F = 1;
	_flag10E = 1;
	startAnimation(0x192ADD30, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	NextState(&AsCommonCar::sub45D390);
}

void AsCommonCar::sub45D390() {
	gotoNextState();
	_flag10F = 1;
	startAnimation(0xF46A0324, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	FinalizeState(&AsCommonCar::sub45D620);
	_value112 = 1;
	sub45CDC0();
}

void AsCommonCar::moveToPrevPoint() {
	if (_currPointIndex == 0 && _stepError == 0) {
		_moreY = 0;
		sendMessage(this, 0x1019, 0);
		sendMessage(_parentScene, 0x2005, 0);
	} else {
		NPoint prevPt;
		NPoint currPt;
		if (_stepError == 0) {
			prevPt = pathPoint(_currPointIndex - 1);
			currPt = pathPoint(_currPointIndex);
		} else {
			prevPt = pathPoint(_currPointIndex);
			currPt = pathPoint(_currPointIndex + 1);
		}
		if (ABS(prevPt.y - currPt.y) <= ABS(prevPt.x - currPt.x) && currPt.x >= prevPt.x &&
			(_currMoveDirection == 2 || _currMoveDirection == 4)) {
			if (_currMoveDirection == 2) 
				_currMoveDirection = 4;
			else if (_currMoveDirection == 4)
				_currMoveDirection = 2;
			if (_flag113)
				sub45D5D0();
			else
				sub45D580();
		} else {
			if (_steps == 0) {
				gotoNextState();
				_flag113 = 0;
				startAnimation(0x9966B138, 0, -1);
				SetMessageHandler(&AsCommonCar::handleMessage45CC30);
				SetUpdateHandler(&AsCommonCar::update);
				NextState(&AsCommonCar::sub45D100);
			}
			_flag10E = 0;
			SetSpriteUpdate(&AsCommonCar::suMoveToPrevPoint);
			_lastDistance = 640;
		}
	}
}

void AsCommonCar::sub45D580() {
	_flag10F = 1;
	_flag10E = 1;
	FinalizeState(NULL);
	startAnimation(0x192ADD30, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	NextState(&AsCommonCar::sub45D5D0);
}

void AsCommonCar::sub45D5D0() {
	_flag10F = 1;
	FinalizeState(NULL);
	startAnimation(0xF46A0324, 0, -1);
	SetMessageHandler(&AsCommonCar::handleMessage45CC30);
	SetUpdateHandler(&AsCommonCar::update);
	FinalizeState(&AsCommonCar::sub45D620);
	_value112 = 2;
	sub45CDC0();
}

void AsCommonCar::sub45D620() {
	_flag10F = 0;
	_newMoveDirection = 0;
	setDoDeltaX(2);
	sub45D100();
}

void AsCommonCar::suMoveToNextPoint() {
	int16 newX = _x, newY = _y;

	if (_currPointIndex >= (int)_pathPoints->size()) {
		_moreY = 0;
		sendMessage(this, 0x1019, 0);
		sendMessage(_parentScene, 0x2006, 0);
		return;
	}

	if (_flag10E) {
		if (_steps <= 0) {
			sendMessage(this, 0x1019, 0);
			return;
		} else {
			_steps--;
		}
	} else if (_steps < 11) {
		_steps++;
	}

	bool firstTime = true;
	_anotherY = _steps;
	int stepsCtr = _steps;
	
	while (stepsCtr > 0) {
		NPoint pt1;
		NPoint pt2 = pathPoint(_currPointIndex);
		if (_currPointIndex + 1 >= (int)_pathPoints->size())
			pt1 = pathPoint(0);
		else
			pt1 = pathPoint(_currPointIndex + 1);
		int16 deltaX = ABS(pt1.x - pt2.x);
		int16 deltaY = ABS(pt1.y - pt2.y);
		if (deltaX >= deltaY) {
			_newMoveDirection = 2;
			if (pt1.x < pt2.x)
				_newMoveDirection = 4;
			if (stepsCtr + _stepError >= deltaX) {
				stepsCtr -= deltaX;
				stepsCtr += _stepError;
				_stepError = 0;
				_currPointIndex++;
				if (_currPointIndex == (int)_pathPoints->size() - 1)
					stepsCtr = 0;
				newX = pathPoint(_currPointIndex).x;
				newY = pathPoint(_currPointIndex).y;
			} else {
				_stepError += stepsCtr;
				if (pt1.x >= pt2.x)
					newX += stepsCtr;
				else
					newX -= stepsCtr;
				if (pt1.y >= pt2.y)
					newY = pt2.y + (deltaY * _stepError) / deltaX;
				else
					newY = pt2.y - (deltaY * _stepError) / deltaX;
				stepsCtr = 0;
			}
		} else {
			_newMoveDirection = 3;
			if (pt1.y < pt2.y)
				_newMoveDirection = 1;
			if (firstTime) {
				if (pt1.y >= pt2.y) {
					stepsCtr += 7;
				} else {
					stepsCtr -= 4;
					if (stepsCtr < 0)
						stepsCtr = 0;
				}
				_anotherY = stepsCtr;
			}
			if (stepsCtr + _stepError >= deltaY) {
				stepsCtr -= deltaY;
				stepsCtr += _stepError;
				_stepError = 0;
				_currPointIndex++;
				if (_currPointIndex == (int)_pathPoints->size() - 1)
					stepsCtr = 0;
				newX = pathPoint(_currPointIndex).x;
				newY = pathPoint(_currPointIndex).y;
			} else {
				_stepError += stepsCtr;
				if (pt1.x >= pt2.x)
					newX = pt2.x + (deltaX * _stepError) / deltaY;
				else
					newX = pt2.x - (deltaX * _stepError) / deltaY;
				if (pt1.y >= pt2.y)
					newY += stepsCtr;
				else
					newY -= stepsCtr;
				stepsCtr = 0;
			}
		}
		firstTime = false;		
	}

	if (_moreY != 0) {
		_x = newX;
		_y = newY;
		_moreY -= _anotherY;
		if (_moreY <= 0) {
			_flag10E = 1;
			_moreY = 0;
		}
	} else {
		int distance = calcDistance(_someX, _someY, _x, _y);
		_x = newX;
		_y = newY;
		if (newX > 20 && newX < 620 && newY > 20 && newY < 460) {
			_exitDirection = 0;
			_field100 = 1;
		} else if (_field100) {
			_someX = pathPoint(_pathPoints->size() - 1).x;
			_someY = pathPoint(_pathPoints->size() - 1).y;
			_field100 = 0;
			if (_x <= 20)
				_exitDirection = 1;
			else if (_x >= 620)
				_exitDirection = 3;
			else if (_y <= 20)
				_exitDirection = 2;
			else if (_y >= 460)
				_exitDirection = 4;
			if (_exitDirection != 0 && _flag10E) {
				_flag10E = 0;
				_steps = 11;
			}
		}
		if ((distance < 20 && _exitDirection == 0 && _lastDistance < distance) ||
			(_exitDirection == 0 && _lastDistance + 20 < distance))
			_flag10E = 1;
		if (distance < _lastDistance)
			_lastDistance = distance;
		if (_currPointIndex == (int)_pathPoints->size() - 1) {
			_flag10E = 1;
			_moreY = 0;
			sendMessage(this, 0x1019, 0);
			sendMessage(_parentScene, 0x2006, 0);
		}
	}

}

void AsCommonCar::suMoveToPrevPoint() {
	int16 newX = _x, newY = _y;

	if (_currPointIndex == 0 && _stepError == 0) {
		_moreY = 0;
		sendMessage(this, 0x1019, 0);
		sendMessage(_parentScene, 0x2005, 0);
		return;
	}

	if (_flag10E) {
		if (_steps <= 0) {
			sendMessage(this, 0x1019, 0);
			return;
		} else {
			_steps--;
		}
	} else if (_steps < 11) {
		_steps++;
	}

	bool firstTime = true;
	_anotherY = _steps;
	int stepsCtr = _steps;
	
	while (stepsCtr > 0) {
		if (_stepError == 0)
			_currPointIndex--;
		NPoint pt1;
		NPoint pt2 = pathPoint(_currPointIndex);
		if (_currPointIndex + 1 >= (int)_pathPoints->size())
			pt1 = pathPoint(0);
		else
			pt1 = pathPoint(_currPointIndex + 1);
		int16 deltaX = ABS(pt1.x - pt2.x);
		int16 deltaY = ABS(pt1.y - pt2.y);
		if (deltaX >= deltaY) {
			_newMoveDirection = 4;
			if (pt1.x < pt2.x)
				_newMoveDirection = 2;
			if (_stepError == 0)
				_stepError = deltaX;
			if (stepsCtr > _stepError) {
				stepsCtr -= _stepError;
				_stepError = 0;
				if (_currPointIndex == 0)
					stepsCtr = 0;
				newX = pathPoint(_currPointIndex).x;
				newY = pathPoint(_currPointIndex).y;
			} else {
				_stepError -= stepsCtr;
				if (pt1.x >= pt2.x)
					newX -= stepsCtr;
				else
					newX += stepsCtr;
				if (pt1.y >= pt2.y)
					newY = pt2.y + (deltaY * _stepError) / deltaX;
				else
					newY = pt2.y - (deltaY * _stepError) / deltaX;
				stepsCtr = 0;
			}
		} else {
			_newMoveDirection = 1;
			if (pt1.y < pt2.y)
				_newMoveDirection = 3;
			if (firstTime) {
				if (pt1.y >= pt2.y) {
					stepsCtr -= 4;
					if (stepsCtr < 0)
						stepsCtr = 0;
				} else {
					stepsCtr += 7;
				}
				_anotherY = stepsCtr;
			}
			if (_stepError == 0)
				_stepError = deltaY;
			if (stepsCtr > _stepError) {
				stepsCtr -= _stepError;
				_stepError = 0;
				if (_currPointIndex == 0)
					stepsCtr = 0;
				newX = pathPoint(_currPointIndex).x;
				newY = pathPoint(_currPointIndex).y;
			} else {
				_stepError -= stepsCtr;
				if (pt1.x >= pt2.x)
					newX = pt2.x + (deltaX * _stepError) / deltaY;
				else
					newX = pt2.x - (deltaX * _stepError) / deltaY;
				if (pt1.y >= pt2.y)
					newY -= stepsCtr;
				else
					newY += stepsCtr;
				stepsCtr = 0;
			}
		}
		firstTime = false;		
	}

	if (_moreY != 0) {
		_x = newX;
		_y = newY;
		_moreY -= _anotherY;
		if (_moreY <= 0) {
			_flag10E = 1;
			_moreY = 0;
		}
	} else {
		int distance = calcDistance(_someX, _someY, _x, _y);
		_x = newX;
		_y = newY;
		if (newX > 20 && newX < 620 && newY > 20 && newY < 460) {
			_exitDirection = 0;
			_field100 = 1;
		} else if (_field100) {
			_someX = pathPoint(0).x;
			_someY = pathPoint(0).y;
			_field100 = 0;
			if (_x <= 20)
				_exitDirection = 1;
			else if (_x >= 620)
				_exitDirection = 3;
			else if (_y <= 20)
				_exitDirection = 2;
			else if (_y >= 460)
				_exitDirection = 4;
			if (_exitDirection != 0 && _flag10E) {
				_flag10E = 0;
				_steps = 11;
			}
		}
		if ((distance < 20 && _exitDirection == 0 && _lastDistance < distance) ||
			(_exitDirection == 0 && _lastDistance + 20 < distance))
			_flag10E = 1;
		if (distance < _lastDistance)
			_lastDistance = distance;
		if (_currPointIndex == 0 && _stepError == 0) {
			_flag10E = 1;
			_moreY = 0;
			sendMessage(this, 0x1019, 0);
			sendMessage(_parentScene, 0x2005, 0);
		}
	}

}

void AsCommonCar::sub45E0A0() {
	// TODO
}

AsScene1608Door::AsScene1608Door(NeverhoodEngine *vm, Scene *parentScene)
	: AnimatedSprite(vm, 0x08C80144, 900, 320, 240), _parentScene(parentScene) {

	setVisible(false);
	SetMessageHandler(&AsScene1608Door::handleMessage);
	stopAnimation();
}

uint32 AsScene1608Door::handleMessage(int messageNum, const MessageParam &param, Entity *sender) {
	uint32 messageResult = Sprite::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x3002:
		gotoNextState();
		break;
	case 0x4808:
		stOpenDoor();
		break; 
	case 0x4809:
		stCloseDoor();
		break; 
	}
	return messageResult;
}

void AsScene1608Door::stOpenDoor() {
	startAnimation(0x08C80144, 0, -1);
	setVisible(true);
	NextState(&AsScene1608Door::stOpenDoorDone);
	playSound(0, calcHash("fxDoorOpen23"));
}

void AsScene1608Door::stOpenDoorDone() {
	sendMessage(_parentScene, 0x2033, 0);
	stopAnimation();
	setVisible(false);
}

void AsScene1608Door::stCloseDoor() {
	startAnimation(0x08C80144, -1, -1);
	setVisible(true);
	NextState(&AsScene1608Door::stCloseDoorDone);
	playSound(0, calcHash("fxDoorClose23"));
}

void AsScene1608Door::stCloseDoorDone() {
	sendMessage(_parentScene, 0x2034, 0);
	stopAnimation();
}

AsCommonIdleCarLower::AsCommonIdleCarLower(NeverhoodEngine *vm, int16 x, int16 y)
	: AnimatedSprite(vm, 0x1209E09F, 1100, x, y) {

	setDoDeltaX(1);
	startAnimation(0x1209E09F, 1, -1);
	_newStickFrameIndex = 1;
}

AsCommonIdleCarFull::AsCommonIdleCarFull(NeverhoodEngine *vm, int16 x, int16 y)
	: AnimatedSprite(vm, 0x1209E09F, 100, x, y) {

	setDoDeltaX(1);
	_newStickFrameIndex = 0;
}

AsCommonCarConnector::AsCommonCarConnector(NeverhoodEngine *vm, AsCommonCar *asCar)
	: AnimatedSprite(vm, 1100), _asCar(asCar) {
	
	SetUpdateHandler(&AsCommonCarConnector::update);
	createSurface1(0x60281C10, 150);
	startAnimation(0x60281C10, -1, -1);
	_newStickFrameIndex = -2;
}

void AsCommonCarConnector::update() {
	_x = _asCar->getX();
	_y = _asCar->getY();
	AnimatedSprite::update();
}

Scene1608::Scene1608(NeverhoodEngine *vm, Module *parentModule, int which)
	: Scene(vm, parentModule, true), _asCar(NULL), _countdown1(0) {

	/* TODO: Find out which of the message/update handlers are used for
		the upper/lower floors and rename them accordingly.
	*/
	
	setGlobalVar(0x21E60190, 1);
	
	_surfaceFlag = true;
	SetMessageHandler(&Scene1608::hmLowerFloor);
	
	_asKey = insertSprite<AsCommonKey>(this, 1, 1100, 198, 220);
	_vm->_collisionMan->addSprite(_asKey);

	if (which < 0) {
		if (_vm->gameState().which == 1)
			which = 1;
		else {
			setRectList(0x004B47D0);
			insertKlayman<KmScene1608>(380, 438);
			_kmScene1608 = _klayman;
			_klaymanInCar = false;
			_asDoor = insertSprite<AsScene1608Door>(this);
			_sprite1 = insertStaticSprite(0x7D0404E8, 1100);
			setMessageList(0x004B46A8);
			setBackground(0x10080E01);
			setPalette(0x10080E01);
			_asTape = insertSprite<AsScene1201Tape>(this, 13, 1100, 412, 443, 0x9148A011);
			_vm->_collisionMan->addSprite(_asTape);
			_klayman->setClipRect(_sprite1->getDrawRect().x, 0, 640, 480);
			SetUpdateHandler(&Scene1608::upLowerFloor);
			insertMouse433(0x80E05108);
			insertStaticSprite(0x4B18F868, 1200);
		}
	} else if (which == 0) {
		_vm->gameState().which = 0;
		setRectList(0x004B47D0);
		insertKlayman<KmScene1608>(0, 438);
		_kmScene1608 = _klayman;
		_klaymanInCar = false;
		setMessageList(0x004B46B0);
		setBackground(0x10080E01);
		setPalette(0x10080E01);
		_asTape = insertSprite<AsScene1201Tape>(this, 13, 1100, 412, 443, 0x9148A011);
		_vm->_collisionMan->addSprite(_asTape);
		insertMouse433(0x80E05108);
		_sprite1 = insertStaticSprite(0x7D0404E8, 1100);
		_asDoor = insertSprite<AsScene1608Door>(this);
		_klayman->setClipRect(_sprite1->getDrawRect().x, 0, 640, 480);
		SetUpdateHandler(&Scene1608::upLowerFloor);
		sendMessage(_asDoor, 0x4808, 0);
		insertStaticSprite(0x4B18F868, 1200);
	} else if (which == 2) {
		_vm->gameState().which = 1;
		_dataResource.load(0x003C0492);
		_roomPathPoints = _dataResource.getPointArray(calcHash("meArchroArchRoomPath"));
		setBackground(0x98001604);
		setPalette(0x98001604);
		_palette->addPalette("paPodRed", 65, 31, 65);
		insertMouse433(0x01600988);
		_sprite2 = insertStaticSprite(0x491F38A8, 1100);
		_asCar = createSprite<AsCommonCar>(this, 375, 227); // Create but don't add to the sprite list yet
		_asIdleCarLower = insertSprite<AsCommonIdleCarLower>(375, 227);
		_asIdleCarFull = insertSprite<AsCommonIdleCarFull>(375, 227);
		_asCar->setVisible(false);
		if (getGlobalVar(0xC0418A02)) {
			insertKlayman<KmScene1608>(373, 220);
			_klayman->setDoDeltaX(1);
		} else {
			insertKlayman<KmScene1608>(283, 220);
		}
		_kmScene1608 = _klayman;
		setMessageList(0x004B47A8);
		SetMessageHandler(&Scene1608::hmUpperFloor);
		SetUpdateHandler(&Scene1608::upUpperFloor);
		// NOTE: Setting the point array was handled by messages 0x2000 (array) and 0x2001 (count) in the original
		_asCar->setPathPoints(_roomPathPoints);
		sendMessage(_asCar, 0x2002, _roomPathPoints->size() - 1);
		_sprite3 = insertStaticSprite(0xB47026B0, 1100);
		_rect1.set(_sprite3->getDrawRect().x, _sprite3->getDrawRect().y, 640, _sprite2->getDrawRect().y2());
		_rect3.set(_sprite2->getDrawRect().x, _sprite3->getDrawRect().y, 640, _sprite2->getDrawRect().y2());
		_rect2 = _rect1;
		_rect2.y2 = 215;
		_klayman->setClipRect(_rect1);
		_asCar->setClipRect(_rect1);
		_asIdleCarLower->setClipRect(_rect1);
		_asIdleCarFull->setClipRect(_rect1);
		_asTape = insertSprite<AsScene1201Tape>(this, 13, 1100, 412, 443, 0x9148A011);
		_vm->_collisionMan->addSprite(_asTape);
		insertSprite<AsCommonCarConnector>(_asCar)->setClipRect(_rect1);
		_klaymanInCar = false;
		_carClipFlag = false;
		_carStatus = 0;
		setRectList(0x004B4810);
	} 

	// NOTE: Not in the else because 'which' is set to 1 in the true branch	
	if (which == 1) {
		_vm->gameState().which = 1;
		_dataResource.load(0x003C0492);
		_roomPathPoints = _dataResource.getPointArray(calcHash("meArchroArchRoomPath"));
		setBackground(0x98001604);
		setPalette(0x98001604);
		_palette->addPalette("paPodRed", 65, 31, 65);
		insertMouse433(0x01600988);
		_asCar = insertSprite<AsCommonCar>(this, 375, 227);
		_asIdleCarLower = insertSprite<AsCommonIdleCarLower>(375, 227);
		_asIdleCarFull = insertSprite<AsCommonIdleCarFull>(375, 227);
		_sprite2 = insertStaticSprite(0x491F38A8, 1100);
		_kmScene1608 = createSprite<KmScene1608>(this, 439, 220); // Special Klayman handling...
		sendMessage(_kmScene1608, 0x2032, 1);
		_kmScene1608->setDoDeltaX(1);
		SetMessageHandler(&Scene1608::hmRidingCar);
		SetUpdateHandler(&Scene1608::upRidingCar);
		_asIdleCarLower->setVisible(false);
		_asIdleCarFull->setVisible(false);
		// NOTE: Setting the point array was handled by messages 0x2000 (array) and 0x2001 (count) in the original
		_asCar->setPathPoints(_roomPathPoints);
		sendMessage(_asCar, 0x2002, 0);
		sendMessage(_asCar, 0x2008, 90);
		_sprite3 = insertStaticSprite(0xB47026B0, 1100);
		_rect1.set(_sprite3->getDrawRect().x, _sprite3->getDrawRect().y, 640, _sprite2->getDrawRect().y2());
		_rect3.set(_sprite2->getDrawRect().x, _sprite3->getDrawRect().y, 640, _sprite2->getDrawRect().y2());
		_rect2 = _rect1;
		_rect2.y2 = 215;
		_kmScene1608->setClipRect(_rect1);
		_asCar->setClipRect(_rect1);
		_asIdleCarLower->setClipRect(_rect1);
		_asIdleCarFull->setClipRect(_rect1);
		_asTape = insertSprite<AsScene1201Tape>(this, 13, 1100, 412, 443, 0x9148A011);
		// ... _vm->_collisionMan->addSprite(_asTape);
		insertSprite<AsCommonCarConnector>(_asCar)->setClipRect(_rect1);
		_klaymanInCar = true;
		_carClipFlag = true;
		_carStatus = 0;
	}
	
	_palette->addPalette("paKlayRed", 0, 64, 0);

}

Scene1608::~Scene1608() {
	setGlobalVar(0xC0418A02, _kmScene1608->isDoDeltaX() ? 1 : 0);
	if (_klaymanInCar) {
		delete _kmScene1608;
	} else {
		delete _asCar;
	}
}

void Scene1608::upLowerFloor() {
	Scene::update();
	if (_countdown1 != 0 && (--_countdown1 == 0)) {
		leaveScene(0);
	}
}

void Scene1608::upUpperFloor() {
	Scene::update();
	if (_carStatus == 1) {
		removeSurface(_klayman->getSurface());
		removeEntity(_klayman);
		addSprite(_asCar);
		_klaymanInCar = true;
		clearRectList();
		SetUpdateHandler(&Scene1608::upCarAtHome);
		SetMessageHandler(&Scene1608::hmCarAtHome);
		_asIdleCarLower->setVisible(false);
		_asIdleCarFull->setVisible(false);
		_asCar->setVisible(true);
		sendMessage(_asCar, 0x2009, 0);
		_asCar->handleUpdate();
		_klayman = NULL;
		_carStatus = 0;
	}
	updateKlaymanCliprect();
}

void Scene1608::upCarAtHome() {
	Scene::update();
	if (_mouseClicked) {
		if (_mouseClickPos.x <= 329 && _asCar->getX() == 375 && _asCar->getY() == 227) {
			sendMessage(_asCar, 0x200A, 0);
			SetUpdateHandler(&Scene1608::upGettingOutOfCar);
		} else {
			sendPointMessage(_asCar, 0x2004, _mouseClickPos);
			SetMessageHandler(&Scene1608::hmRidingCar);
			SetUpdateHandler(&Scene1608::upRidingCar);
		}
		_mouseClicked = false;
	}
	updateKlaymanCliprect();
}

void Scene1608::upGettingOutOfCar() {
	Scene::update();
	if (_carStatus == 2) {
		_klayman = _kmScene1608;
		removeSurface(_asCar->getSurface());
		removeEntity(_asCar);
		addSprite(_klayman);
		_klaymanInCar = false;
		SetMessageHandler(&Scene1608::hmUpperFloor);
		SetUpdateHandler(&Scene1608::update);
		setRectList(0x004B4810);
		_asIdleCarLower->setVisible(true);
		_asIdleCarFull->setVisible(true);
		_asCar->setVisible(false);
		setMessageList(0x004B4748);
		processMessageList();
		_klayman->handleUpdate();
		_carStatus = 0;
	}
	updateKlaymanCliprect();
}

void Scene1608::upRidingCar() {
	Scene::update();
	if (_mouseClicked) {
		sendPointMessage(_asCar, 0x2004, _mouseClickPos);
		_mouseClicked = false;
	}
	if (_asCar->getX() < 300) {
		if (_carClipFlag) {
			_carClipFlag = false;
			_asCar->setClipRect(_rect1);
			if (!_asCar->isDoDeltaX())
				sendMessage(_asCar, 0x200E, 0);
		}
	} else if (!_carClipFlag) {
		_carClipFlag = true;
		_asCar->setClipRect(_rect3);
	}
}

uint32 Scene1608::hmLowerFloor(int messageNum, const MessageParam &param, Entity *sender) {
	Scene::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x100D:
		if (param.asInteger() == 0x20250B1A) {
			clearRectList();
			_klayman->setVisible(false);
			showMouse(false);
			_sprite1->setVisible(false);
			sendMessage(_asDoor, 0x4809, 0);
			_countdown1 = 28;
		}
		break;
	case 0x200D:
		sendMessage(_parentModule, 0x200D, 0);
		break;
	case 0x4826:
		if (sender == _asTape) {
			sendEntityMessage(_kmScene1608, 0x1014, _asTape);
			setMessageList(0x004B4770);
		} else if (sender == _asKey) {
			setMessageList(0x004B46C8);
		}
		break;
	}
	return 0;
}

uint32 Scene1608::hmUpperFloor(int messageNum, const MessageParam &param, Entity *sender) {
	Scene::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x100D:
		if (param.asInteger() == 0x60842040) {
			_carStatus = 1;
		}
		break;
	case 0x200D:
		sendMessage(_parentModule, 0x200D, 0);
		break;
	case 0x4826:
		if (sender == _asKey) {
			sendEntityMessage(_kmScene1608, 0x1014, _asKey);
			setMessageList(0x004B4760);
		}
		break;
	}
	return 0;
}

uint32 Scene1608::hmRidingCar(int messageNum, const MessageParam &param, Entity *sender) {
	Scene::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x2005:
		leaveScene(1);
		break;
	case 0x2006:
		SetMessageHandler(&Scene1608::hmCarAtHome);
		SetUpdateHandler(&Scene1608::upCarAtHome);
		sendMessage(_asCar, 0x200F, 1);
		break;
	case 0x200D:
		sendMessage(_parentModule, 0x200D, 0);
		break;
	}
	return 0;
}

uint32 Scene1608::hmCarAtHome(int messageNum, const MessageParam &param, Entity *sender) {
	Scene::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x200A:
		_carStatus = 2;
		break;
	case 0x200D:
		sendMessage(_parentModule, 0x200D, 0);
		break;
	}
	return 0;
}
	
void Scene1608::updateKlaymanCliprect() {
	if (_kmScene1608->getX() <= 375)
		_kmScene1608->setClipRect(_rect1);
	else
		_kmScene1608->setClipRect(_rect2);
}

Scene1609::Scene1609(NeverhoodEngine *vm, Module *parentModule, int which)
	: Scene(vm, parentModule, true), _countdown1(1), _currentSymbolIndex(0), _symbolPosition(0), _changeCurrentSymbol(true), _isSolved(false) {

	// TODO _vm->gameModule()->initScene3011Vars();
	_noisySymbolIndex = getGlobalVar(0x2414C2F2);
	
	_surfaceFlag = true;
	SetMessageHandler(&Scene1609::handleMessage);
	SetUpdateHandler(&Scene1609::update);
	
	setBackground(0x92124A14);
	setPalette(0x92124A14);
	
	for (int symbolPosition = 0; symbolPosition < 12; symbolPosition++)
		_asSymbols[symbolPosition] = insertSprite<AsScene3011Symbol>(symbolPosition, false);
	
	_ssButton = insertSprite<SsScene3011Button>(this, true);
	_vm->_collisionMan->addSprite(_ssButton);

	insertMouse435(0x24A10929, 20, 620);

	loadSound(0, 0x68E25540);

}

void Scene1609::update() {
	if (!_isSolved && _countdown1 != 0 && (--_countdown1 == 0)) {
		if (_changeCurrentSymbol) {
			_currentSymbolIndex++;
			if (_currentSymbolIndex >= 12)
				_currentSymbolIndex = 0;
			_asSymbols[_symbolPosition]->change(_currentSymbolIndex + 12, _currentSymbolIndex == (int)getSubVar(0x04909A50, _noisySymbolIndex));
			_changeCurrentSymbol = false;
			_countdown1 = 36;
		} else {
			_asSymbols[_symbolPosition]->hide();
			_changeCurrentSymbol = true;
			_countdown1 = 12;
		}
	}
	if (_isSolved && !isSoundPlaying(0)) {
		leaveScene(1);
	}
	Scene::update();
}

uint32 Scene1609::handleMessage(int messageNum, const MessageParam &param, Entity *sender) {
	Scene::handleMessage(messageNum, param, sender);
	switch (messageNum) {
	case 0x0001:
		// TODO: Debug stuff
		if (param.asPoint().x <= 20 || param.asPoint().x >= 620)
			leaveScene(0);
		break;
	// TODO: Debug stuff
	case 0x2000:
		if (!_isSolved) {
			if (_changeCurrentSymbol)
				_asSymbols[_symbolPosition]->change(_currentSymbolIndex + 12, false);
			_asSymbols[_symbolPosition]->stopSymbolSound();
			_symbolPosition++;
			if (_symbolPosition >= 12) {
				if (testVars()) {
					playSound(0);
					setGlobalVar(0x2C531AF8, 1);
					_isSolved = true;
				} else {
					_symbolPosition = 0;
					for (int i = 0; i < 12; i++)
						_asSymbols[i]->hide();
				}
			}
			_changeCurrentSymbol = true;
			_countdown1 = 1;
		}
		break;
	}
	return 0;
}

bool Scene1609::testVars() {
	int cmpSymbolIndex = 0;

	// Find the position of the first symbol
	while ((int)getSubVar(0x04909A50, cmpSymbolIndex) != _asSymbols[0]->getSymbolIndex())
		cmpSymbolIndex++;

	// Check if the entered symbols match
	for (int enteredSymbolIndex = 0; enteredSymbolIndex < 12; enteredSymbolIndex++) {
		if ((int)getSubVar(0x04909A50, cmpSymbolIndex) != _asSymbols[enteredSymbolIndex]->getSymbolIndex())
			return false;
		cmpSymbolIndex++;
		if (cmpSymbolIndex >= 12)
			cmpSymbolIndex = 0;
	}

	return true;
}
	
} // End of namespace Neverhood
