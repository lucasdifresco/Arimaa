
/*
 * timecontrol.h
 * Author: davidwu
 */

#ifndef TIMECONTROL_H
#define TIMECONTROL_H

struct TimeControl
{
  //Fixed per game
  int perMove;          //Seconds per move
  int percent;          //Percent of leftover time saved into reserve
  int reserveStart;     //Initial reserve time
  int reserveMax;       //Maximum reserve time
  int gameCurrent;      //Current time used for game
  int gameMax;          //Total time allowed for game
  int perMoveMax;       //Max time allowed for a move
  int alreadyUsed;      //How much time already used for this move?

  //Dynamic
  double reserveCurrent;   //Current reserve time

  inline TimeControl()
  {
    perMove = 30;
    percent = 100;
    reserveStart = 90;
    reserveMax = 180;
    gameCurrent = 0;
    gameMax = 86400;
    perMoveMax = 86400;
    alreadyUsed = 0;

    reserveCurrent = 90;
  }
};

#endif
