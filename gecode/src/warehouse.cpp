/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main author:
 *     Michael Brendle <michaelbrendle@me.com>
 *
 */

#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

#include "jsoncpp/jsoncpp.cpp"

#include "jsoncpp/json/json.h"
#include "jsoncpp/json/json-forwards.h"

#include <fstream>

using namespace std;
using namespace Gecode;
using namespace Gecode::Driver;

/// Global variables for reading information from the database

/// TODO: number of robots

/// Number of goods.
int __numGoods;

int __robotStartPosition;
int __robotStartOrientation;

/// Start position of the all goods
IntArgs __goodsStartingPosition;

/// Minimum and Maximum Temperatur of goods.
IntArgs __goodsTempMin;
IntArgs __goodsTempMax;
/// Minimum and Maximum Light of goods.
IntArgs __goodsLightMin;
IntArgs __goodsLightMax;

/// Number of Warehouse places.
int __numWarehouses;
/// Temperatur of Warehouses.
IntArgs __warehouseTemp;
/// Light of Warehouses.
IntArgs __warehouseLight;
/// Position of the Warehouse
IntArgs __warehousePosition;

/// Number of garage places.
int __numGarage;
/// Position of the garage places
IntArgs __garagePosition;

string __final_output = "";


IntArgs __goodsEndPositions;
int __robotFinalPosition;
int __robotFinalOrientation;


bool __robotBoolMoving = false;
int __robotBoolMovingPos;

bool __robotBoolPlaceGood = false;
int __robotBoolPlaceGoodFromPos;
int __robotBoolPlaceGoodNumber;
int __robotBoolPlaceGoodToPos;

bool __robotBoolAddGood = false;

bool __robotBoolDropGood = false;
int __robotBoolDropGoodFromPos;
int __robotBoolDropGoodNumber;

bool __boolDummyGood = false;

bool __robotLastBackwardBefore = false;
bool __robotLastBackwardAfter = false;

bool __foundSolution = false;


class Warehouse : public IntMinimizeScript {
protected:

  /// Number of maximal robot tasks.
  static const int maxTasks = 16;
  /// Array of robotTasks and the corresponding BoolArray.
  IntVarArray robotTasks;
  BoolVarArray robotTasksBoolArray;

  /// Array of the cost for each task.
  IntVarArray robotTasksCost;
  /// Linear all RobotTaskCost to robotCost.
  IntVar robotCost;

  /// TODO: warehouse costs: maybe 10
  /// TODO: garage costs: maybe 5

  /// Array of robotPositions at start and end (0 to 48) and corresponding
  /// BoolArray at start.
  /// The positions will be encoded like that (this is the Warehouse).
  ///
  ///    42 ------------------------- 48
  ///    35 ------------------------- 41
  ///    28 ------------------------- 34
  ///    21 ------------------------- 27
  ///    14 ------------------------- 20
  ///    7 ------------ 10 ---------- 13
  ///    0 -- 1 -- 2 -- 3 -- 4 -- 5 -- 6
  ///
  /// Note that also all inner values, like 8, 9, 11, 12, 15, ..., 47
  /// are possible.
  ///
  IntVarArray robotPositionsStart;
  IntVarArray robotPositionsEnd;
  BoolVarArray robotPositionsStartBoolArray;

  /// Array of the difference of the robot position from start to end of
  /// the task and the temp difference.
  IntVarArray robotPositionsDiff;
  IntVarArray robotPositionsDiffTemp;

  /// Array of how many steps the robot will do in this task.
  IntVarArray robotMovingForward;

  /// Array of robotOrientation: N = 0, E = 1, S = 2, W = 3 at start and end
  /// and corresponding BoolArray.
  IntVarArray robotOrientationStart;
  BoolVarArray robotOrientationStartBoolArray;
  IntVarArray robotOrientationEnd;

  /// Array of the difference of the robot orientation from start to end of
  /// the task and the new position end value without mod.
  IntVarArray robotOrientDiff;
  IntVarArray robotOrientMod;

  /// Array of Goods at the robot at start and at end of the task.
  IntVarArray robotGoodsStart;
  IntVarArray robotGoodsEnd;

  /// Matrix/Array of the positions of the goods at start and at the end.
  IntVarArray goodsPositionStartArray;
  IntVarArray goodsPositionEndArray;

  // Goods Penalty cost
  IntVarArray goodsPenaltyCost;
  // Totsl Penalty Cost
  IntVar penaltyCost;



  /// Total cost (want to minimize, doing that with branching).
  IntVar c;

public:
  /// Actual model
  Warehouse(const Options& opt) : IntMinimizeScript(opt),
  robotTasks(*this,maxTasks,0,4),
  robotTasksBoolArray(*this,maxTasks*5,0,1),
  robotTasksCost(*this,maxTasks,0,1),
  robotCost(*this,0,100),
  robotPositionsStart(*this,maxTasks,0,48),
  robotPositionsStartBoolArray(*this,maxTasks*49,0,1),
  robotPositionsEnd(*this,maxTasks,0,48),
  robotPositionsDiff(*this,maxTasks,-42,42),
  robotPositionsDiffTemp(*this,maxTasks,-7,7),
  robotMovingForward(*this,maxTasks,-1,6),
  robotOrientationStart(*this,maxTasks,0,3),
  robotOrientationStartBoolArray(*this,maxTasks*4,0,1),
  robotOrientationEnd(*this,maxTasks,0,3),
  robotOrientDiff(*this, maxTasks,-1,1),
  robotOrientMod(*this,maxTasks,0,9),
  robotGoodsStart(*this,maxTasks,-1,__numGoods-1), // -1 no goods
  robotGoodsEnd(*this,maxTasks,-1,__numGoods-1), // -1 no goods
  goodsPositionStartArray(*this,maxTasks*__numGoods,0,48),
  goodsPositionEndArray(*this,maxTasks*__numGoods,0,48),
  goodsPenaltyCost(*this,__numGoods,0,100),
  penaltyCost(*this,0,1600),
  c(*this,0,1700)
    {


      // TODO: FORWARD, 1,* row, column;
      // TODO: TURN, 1, RIGHT, 0
      // TODO: TURN, 2, RIGHT, 1
      // TODO: TURN, 1, LEFT, 0
      // TODO: TURN, 2, LEFT, 1


      /// SETTING UP MATRIZES

      // Setting up the two Matrizes for the positions of the goods at the
      // start and at the end of the task.
      Matrix<IntVarArray> goodsPositionStart(goodsPositionStartArray,maxTasks,__numGoods);
      Matrix<IntVarArray> goodsPositionEnd(goodsPositionEndArray,maxTasks,__numGoods);

      // Matrix for the Boolean of the tasks.
      Matrix<BoolVarArray> robotTasksBool(robotTasksBoolArray,maxTasks,5);

      // Matrix for the Boolean for the positions of the robot at start of the
      // task.
      Matrix<BoolVarArray> robotPositionsStartBool(robotPositionsStartBoolArray,maxTasks,49);

      // Matrix for the orientation of the robot at start of the task.
      Matrix<BoolVarArray> robotOrientationStartBool(robotOrientationStartBoolArray,maxTasks,4);




      /// TASKS

      // These are the five tasks:
      //
      // Task 0: Do nothing (only at the end)
      // Task 1: Turning (Right or Left)
      // Task 2: Moving (-1 to 6) Note that -1 is a Backward move
      // Task 3: Pick up
      // Task 4: Drop

      // JobCost for the five tasks: Only the do nothing task cost nothing.
      // All other tasks cost exactly 1 at the moment. Could be changed
      // later for optimization.
      IntArgs jobCost(5,  0, 1, 1, 1, 1);

      // Help Array for the difference per step. The index of the array
      // represents the orientation of the robot.
      // E.g. is the orientation north, then for the robot one step is adding
      // 7 to the current position, like from position 14 to 21.
      IntArgs jobMovingOrientation(5,  7, 1, -7, -1, 0);

      // Help Array for the 16 Warehouse fields.
      int warehouseFields [16] = {8,9,11,12,15,16,18,19,29,30,32,33,36,37,39,40};

      // Help Array for the 12 finish warehouse places. Here are not included
      // the four places at the start.
      int finishWarehouseFields [12] = {11,12,18,19,29,30,32,33,36,37,39,40};

      // Garage fields - there is no temperature and light sensor
      int garageFields [4] = {8,9,15,16};

      // Help Array for the 33 street fields.
      int streetFields [33] = {0,1,2,3,4,5,6,7,10,13,14,17,20,21,22,23,24,25,26,27,28,31,34,35,38,41,42,43,44,45,46,47,48};




      /// START CONSTRAINTS

      // TODO: Robot Orientation and Position
      //       Will be read later from the database

      // Start position and orientation of the robot
      rel(*this, robotPositionsStart[0] == __robotStartPosition);
      rel(*this, robotOrientationStart[0] == __robotStartOrientation);
      // The current good at the robot; -1 means that no good is at the robot;
      // Otherwise 0 to __numGoods-1
      rel(*this, robotGoodsStart[0] == -1);



      // The starting position from the goods
      for (int j = 0; j < __numGoods; j++) {
          rel(*this, goodsPositionStart(0,j) == __goodsStartingPosition[j]);
      }




      /// CONSTRAINTS FOR EACH TASK

      for (int i = 0; i < maxTasks; i++) {

          /// END - START CONSTRAINTS

          if (i < maxTasks - 1) {

            // End Position of task i is the start position of task i + 1
            // This holds for the robot and for the goods.
            rel(*this, robotPositionsEnd[i] == robotPositionsStart[i+1]);
            rel(*this, robotOrientationEnd[i] == robotOrientationStart[i+1]);
            for (int j = 0; j < __numGoods; j++) {
                rel(*this, goodsPositionEnd(i,j) == goodsPositionStart(i+1,j));
            }

            // The good at the robot at the end of task i is the same good
            // at the robot at task i + 1.
            rel(*this, robotGoodsEnd[i] == robotGoodsStart[i+1]);

          }

          // Each good is at a different position in the Warehouse.
          distinct(*this, goodsPositionStart.col(i));
          distinct(*this, goodsPositionEnd.col(i));

          // Channel the robot Tasks to a Boolean for each Task.
          channel(*this, robotTasksBool.col(i), robotTasks[i]);
          // All Boolean Tasks must sum up to 1.
          linear(*this, robotTasksBool.col(i), IRT_EQ, 1);

          // Channel robot Positions.
          channel(*this, robotPositionsStartBool.col(i), robotPositionsStart[i]);
          // All Boolean Tasks must sum up to 1.
          linear(*this, robotPositionsStartBool.col(i), IRT_EQ, 1);

          // channel robot Orientation and they sum up to 1.
          channel(*this, robotOrientationStartBool.col(i), robotOrientationStart[i]);
          // All Boolean Tasks must sum up to 1.
          linear(*this, robotOrientationStartBool.col(i), IRT_EQ, 1);




          /// TASK 1: TURNING

          // Help array: -1 Turn left; +1 Turn right.
          int domainBool [2] = {-1, 1};

          // Orientation at the start + the difference + 4 is the
          // orientation for the robot at the end without modulo.
          rel(*this, robotOrientationStart[i] + robotOrientDiff[i] + 4 == robotOrientMod[i]);

          // Adding the modulo constraint for the orientation of the robot at
          // the end.
          mod(*this, robotOrientMod[i], IntVar(*this, 4, 4), robotOrientationEnd[i]);

          // If the current task is a Turning Task (== Task 1), the difference
          // of the robot orentation will be -1 or 1. Otherwise 0, since there
          // is no other turnings in the other tasks.
          ite(*this, robotTasksBool(i,1), IntVar(*this, IntSet(domainBool, 2)), IntVar(*this,0,0), robotOrientDiff[i]);

          // There will be no two turns in a row. We do not need a 180 or
          // 270 degree turn.
          if (i < maxTasks-1) {
              rel(*this, robotTasksBool(i,1) + robotTasksBool(i+1,1) < 2);
          }




          /// TASK 2: MOVING

          // The start position + the difference between start and end is the
          // end position.
          rel(*this, robotPositionsStart[i] + robotPositionsDiff[i] == robotPositionsEnd[i]);

          // The difference is the number of steps (-1 to 6) * the difference
          // for one step (that is dependent on the current orientation of the
          // robot).
          rel(*this, robotPositionsDiff[i] == robotPositionsDiffTemp[i] * robotMovingForward[i]);

          // Help Array for the possible moving steps (everything from -1 to
          // 6, without the 0).
          int possibleMovingSteps [7] = {-1, 1, 2, 3, 4, 5, 6};

          // If the current task is a moving task, then the number of steps will
          // be a value from this help array.
          ite(*this, robotTasksBool(i,2), IntVar(*this, IntSet(possibleMovingSteps, 7)), IntVar(*this, 0, 0), robotMovingForward[i]);

          // Checking Moving Forward
          IntVar definedRobotOrientation(*this, 0, 4);

          // If the current task is a moving task, then the value of
          // definedRobotOrientation is between 0 and 3, otherwise 4. this
          // 4 is needed for the case that the current task is not a moving,
          // because then the difference will be 0.
          ite(*this, robotTasksBool(i,2), IntVar(*this, 0, 3), IntVar(*this, 4, 4), definedRobotOrientation);

          // If the current task is a moving task, then the value of
          // definedRobotOrientation is the same as the orientation of the
          // robot from the start of the task.
          ite(*this, robotTasksBool(i,2), robotOrientationStart[i], IntVar(*this, 4, 4), definedRobotOrientation);

          // Finally, the difference for one step for the robot is the same
          // as the value at the index of the current orientation of the
          // robot from the help array jobMovingOrientation:
          //    jobMovingOrientation(5,  7, 1, -7, -1, 0);
          element(*this, jobMovingOrientation, definedRobotOrientation, robotPositionsDiffTemp[i]);

          // If the current task is a backward moving, then the start of the
          // task must be in one of the 16 warehouse places.
          ite(*this, expr(*this, robotMovingForward[i] == IntVar(*this, -1, -1)), IntVar(*this, IntSet(warehouseFields,16)), IntVar(*this, 0, 48), robotPositionsStart[i]);

          // If the current task is a moving, then the robot position at the
          // start and at the end must be in one of the street fields, since
          // the robot can not turn inside a warehouse field.
          ite(*this, robotTasksBool(i,1), IntVar(*this, IntSet(streetFields,33)), IntVar(*this, 0, 48), robotPositionsStart[i]);
          ite(*this, robotTasksBool(i,1), IntVar(*this, IntSet(streetFields,33)), IntVar(*this, 0, 48), robotPositionsEnd[i]);

          // No two Movings in a row.
          if (i < maxTasks-1) {
              rel(*this, robotTasksBool(i,2) + robotTasksBool(i+1,2) < 2);
          }

          // Movement of the goods:
          //
          // By iterating over all goods, if the current task is a moving task
          // and the current good at the robot is the current good in this
          // iteration, then the end position of this current good in this
          // task will be the same as the end position of the robot at this
          // task. Otherwise, the good is not moving in this task, and then
          // the end position at this task will be the same as the start
          // position  of this good.
          for (int j=0; j < __numGoods; j++) {
            ite(*this, expr(*this, !(robotTasksBool(i,2) && (robotGoodsStart[i] == IntVar(*this, j, j)))), goodsPositionStart(i,j), robotPositionsEnd[i], goodsPositionEnd(i,j));
          }




          /// TASK 3: PICKING UP

          // Help IntVars:
          IntVar definedRobotGoodsEnd(*this, 0, __numGoods - 1);
          IntVar definedRobotPositionsEnd(*this, 0, 48);

          // If the current task is a picking up task, then the robot must
          // have no goods at the start of this task. Otherwise, the
          // robot could have goods or not.
          ite(*this, robotTasksBool(i,3), IntVar(*this,-1,-1), IntVar(*this,-1,__numGoods-1), robotGoodsStart[i]);

          // If the current task is a picking up task, then the good of the
          // robot is stored in the help IntVar definedRobotGoodsEnd.
          ite(*this, robotTasksBool(i,3), definedRobotGoodsEnd, IntVar(*this, -1, __numGoods-1), robotGoodsEnd[i]);

          // If the current task is a picking up task, then the end position
          // of the robot is stored in the help IntVar definedRobotPositionsEnd.
          ite(*this, robotTasksBool(i,3), definedRobotPositionsEnd, IntVar(*this, 0, 48), robotPositionsEnd[i]);

          // Finally, we are searching with the element constraint for the
          // number of the good that is at the same position as the robot
          // at the moment.
          element(*this, goodsPositionStart.col(i), definedRobotGoodsEnd, definedRobotPositionsEnd);




          /// TASK 4: DROPPING

          // If the current task is a dropping task, then the robot must
          // have no good at the end. Otherwise, it can be anything.
          ite(*this, robotTasksBool(i,4), IntVar(*this,-1,-1), IntVar(*this,-1,__numGoods-1), robotGoodsEnd[i]);

          // If the current task is a dropping task, then the robot must have
          // a good at the start of the task. Otherwise, it can be anything.
          ite(*this, robotTasksBool(i,4), IntVar(*this,0,__numGoods-1), IntVar(*this,-1,__numGoods-1), robotGoodsStart[i]);




          /// TASK 3 OR 4: PICKING OR DROPPING

          // If the current task is not a picking or a dropping task, then
          // the good at the robot will be the same before and after the task.
          ite(*this, expr(*this, robotTasksBool(i,3) || robotTasksBool(i,4)), IntVar(*this, -1, __numGoods-1), robotGoodsStart[i], robotGoodsEnd[i]);


          // Before a picking or a dropping there must be a forward 1 move.
          if (i > 1) {
            ite(*this, robotTasksBool(i,4), IntVar(*this, 2, 2), IntVar(*this, 0, 4), robotTasks[i-1]);
            ite(*this, robotTasksBool(i,4), IntVar(*this, 1, 1), IntVar(*this, -1, 6), robotMovingForward[i-1]);

            ite(*this, robotTasksBool(i,3), IntVar(*this, 2, 2), IntVar(*this, 0, 4), robotTasks[i-1]);
            ite(*this, robotTasksBool(i,3), IntVar(*this, 1, 1), IntVar(*this, -1, 6), robotMovingForward[i-1]);
          }

          // After a picking or a dropping there must be a backward movement.
          if (i < maxTasks-1) {
            ite(*this, robotTasksBool(i,4), IntVar(*this, 2, 2),  IntVar(*this, 0, 4), robotTasks[i+1]);
            ite(*this, robotTasksBool(i,4), IntVar(*this, -1, -1), IntVar(*this, -1, 6), robotMovingForward[i+1]);

            ite(*this, robotTasksBool(i,3), IntVar(*this, 2, 2),  IntVar(*this, 0, 4), robotTasks[i+1]);
            ite(*this, robotTasksBool(i,3), IntVar(*this, -1, -1), IntVar(*this, -1, 6), robotMovingForward[i+1]);
          } else {

            // Last job can not be a picking or a dropping.
            rel(*this, robotTasks[i] != IntVar(*this, 4,4));
            rel(*this, robotTasksBool(i,4) == false);

            rel(*this, robotTasks[i] != IntVar(*this, 3,3));
            rel(*this, robotTasksBool(i,3) == false);
          }

          // Picking up and dropping only in the Warehouse fields.
          ite(*this, robotTasksBool(i,3), IntVar(*this, IntSet(warehouseFields,16)), IntVar(*this, 0, 48), robotPositionsStart[i]);
          ite(*this, robotTasksBool(i,3), IntVar(*this, IntSet(warehouseFields,16)), IntVar(*this, 0, 48), robotPositionsEnd[i]);

          ite(*this, robotTasksBool(i,4), IntVar(*this, IntSet(warehouseFields,16)), IntVar(*this, 0, 48), robotPositionsStart[i]);
          ite(*this, robotTasksBool(i,4), IntVar(*this, IntSet(warehouseFields,16)), IntVar(*this, 0, 48), robotPositionsEnd[i]);

          // No two picking or dropping in a row.
          if (i < maxTasks-1) {
              rel(*this, robotTasksBool(i,3) + robotTasksBool(i,4) + robotTasksBool(i+1,3) + robotTasksBool(i+1,4) < 2 );
          }

          // TODO: after a picking or a dropping return 2 instead of 1,
          // can be handled at the print out.




          /// NOT ALLOWED MOVEMENTS

          /// TODO: format them in a more symmetric way.

          // If the orientation from the robot is to North = 0.
          ite(*this, expr(*this,robotPositionsStartBool(i,0) && robotOrientationStartBool(i,0)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,3) && robotOrientationStartBool(i,0)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,6) && robotOrientationStartBool(i,0)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,1) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,2) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,4) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,5) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,7) && robotOrientationStartBool(i,0)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,10) && robotOrientationStartBool(i,0)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,13) && robotOrientationStartBool(i,0)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,8) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,9) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,11) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,12) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,14) && robotOrientationStartBool(i,0)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,17) && robotOrientationStartBool(i,0)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,20) && robotOrientationStartBool(i,0)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,15) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,16) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,18) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,19) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,21) && robotOrientationStartBool(i,0)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,24) && robotOrientationStartBool(i,0)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,27) && robotOrientationStartBool(i,0)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,22) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,23) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,25) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,26) && robotOrientationStartBool(i,0)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,28) && robotOrientationStartBool(i,0)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,31) && robotOrientationStartBool(i,0)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,34) && robotOrientationStartBool(i,0)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,29) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,30) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,32) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,33) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,35) && robotOrientationStartBool(i,0)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,38) && robotOrientationStartBool(i,0)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,41) && robotOrientationStartBool(i,0)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,36) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,37) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,39) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,40) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,42) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,45) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,48) && robotOrientationStartBool(i,0)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,43) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,44) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,46) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,47) && robotOrientationStartBool(i,0)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);



          // If the orientation from the robot is to East = 1.
          ite(*this, expr(*this,robotPositionsStartBool(i,0) && robotOrientationStartBool(i,1)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,21) && robotOrientationStartBool(i,1)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,42) && robotOrientationStartBool(i,1)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,7) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,14) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,28) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,35) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,1) && robotOrientationStartBool(i,1)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,22) && robotOrientationStartBool(i,1)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,43) && robotOrientationStartBool(i,1)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,8) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,15) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,29) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,36) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,2) && robotOrientationStartBool(i,1)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,23) && robotOrientationStartBool(i,1)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,44) && robotOrientationStartBool(i,1)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,9) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,16) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,30) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,37) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,3) && robotOrientationStartBool(i,1)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,24) && robotOrientationStartBool(i,1)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,45) && robotOrientationStartBool(i,1)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,10) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,17) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,31) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,38) && robotOrientationStartBool(i,1)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,4) && robotOrientationStartBool(i,1)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,25) && robotOrientationStartBool(i,1)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,46) && robotOrientationStartBool(i,1)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,11) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,18) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,32) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,39) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,5) && robotOrientationStartBool(i,1)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,26) && robotOrientationStartBool(i,1)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,47) && robotOrientationStartBool(i,1)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,12) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,19) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,33) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,40) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,6) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,27) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,48) && robotOrientationStartBool(i,1)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,13) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,20) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,34) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,41) && robotOrientationStartBool(i,1)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);



          // If the orientation from the robot is to South = 2.
          ite(*this, expr(*this,robotPositionsStartBool(i,42) && robotOrientationStartBool(i,2)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,45) && robotOrientationStartBool(i,2)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,48) && robotOrientationStartBool(i,2)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,43) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,44) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,46) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,47) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,35) && robotOrientationStartBool(i,2)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,38) && robotOrientationStartBool(i,2)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,41) && robotOrientationStartBool(i,2)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,36) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,37) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,39) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,40) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,28) && robotOrientationStartBool(i,2)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,31) && robotOrientationStartBool(i,2)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,34) && robotOrientationStartBool(i,2)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,29) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,30) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,32) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,33) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,21) && robotOrientationStartBool(i,2)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,24) && robotOrientationStartBool(i,2)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,27) && robotOrientationStartBool(i,2)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,22) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,23) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,25) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,26) && robotOrientationStartBool(i,2)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,14) && robotOrientationStartBool(i,2)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,17) && robotOrientationStartBool(i,2)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,20) && robotOrientationStartBool(i,2)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,15) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,16) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,18) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,19) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,7) && robotOrientationStartBool(i,2)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,10) && robotOrientationStartBool(i,2)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,13) && robotOrientationStartBool(i,2)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,8) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,9) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,11) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,12) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,0) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,3) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,6) && robotOrientationStartBool(i,2)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,1) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,2) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,4) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,5) && robotOrientationStartBool(i,2)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);



          // If the orientation from the robot is to West = 3.
          ite(*this, expr(*this,robotPositionsStartBool(i,6) && robotOrientationStartBool(i,3)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,27) && robotOrientationStartBool(i,3)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,48) && robotOrientationStartBool(i,3)), IntVar(*this, 0,6), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,13) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,20) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,34) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,41) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,5) && robotOrientationStartBool(i,3)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,26) && robotOrientationStartBool(i,3)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,47) && robotOrientationStartBool(i,3)), IntVar(*this, -1,5), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,12) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,19) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,33) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,40) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,4) && robotOrientationStartBool(i,3)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,25) && robotOrientationStartBool(i,3)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,46) && robotOrientationStartBool(i,3)), IntVar(*this, -1,4), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,11) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,18) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,32) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,39) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,3) && robotOrientationStartBool(i,3)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,24) && robotOrientationStartBool(i,3)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,45) && robotOrientationStartBool(i,3)), IntVar(*this, -1,3), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,10) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,17) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,31) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,38) && robotOrientationStartBool(i,3)), IntVar(*this, 0,1), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,2) && robotOrientationStartBool(i,3)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,23) && robotOrientationStartBool(i,3)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,44) && robotOrientationStartBool(i,3)), IntVar(*this, -1,2), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,9) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,16) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,30) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,37) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,1) && robotOrientationStartBool(i,3)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,22) && robotOrientationStartBool(i,3)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,43) && robotOrientationStartBool(i,3)), IntVar(*this, -1,1), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,8) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,15) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,29) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,36) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);


          ite(*this, expr(*this,robotPositionsStartBool(i,0) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,21) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,42) && robotOrientationStartBool(i,3)), IntVar(*this, -1,0), IntVar(*this,-1,6), robotMovingForward[i]);

          ite(*this, expr(*this,robotPositionsStartBool(i,7) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,14) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,28) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);
          ite(*this, expr(*this,robotPositionsStartBool(i,35) && robotOrientationStartBool(i,3)), IntVar(*this, 0,0), IntVar(*this,-1,6), robotMovingForward[i]);





          /// END CONSTRAINTS

          // The robot should have no good at the end.
          rel(*this, robotGoodsEnd[maxTasks-1] == -1);

          if (__robotBoolMoving) {
              rel(*this, robotPositionsEnd[maxTasks-1] == __robotBoolMovingPos);
          } else if (__robotBoolPlaceGood) {
              rel(*this, goodsPositionEnd(maxTasks-1,__robotBoolPlaceGoodNumber) == __robotBoolPlaceGoodToPos);
          } else if (__robotBoolAddGood) {
              rel(*this, goodsPositionEnd(maxTasks-1,__numGoods-1) != 8);
              rel(*this, goodsPositionEnd(maxTasks-1,__numGoods-1) != 15);
          } else if (__robotBoolDropGood) {
              rel(*this, goodsPositionEnd(maxTasks-1,__robotBoolDropGoodNumber) == 15);
          }

          // TODO: otherwise, add and drop spaces should be empty

          /*

          // TODO: EDIT Garage Warehouse Places must be empty at the end -> No Goods Inside
          if (i == maxTasks-1) {
            for (int j = 0; j < __numGoods; j++) {
                rel(*this, goodsPositionEnd(i,j) == IntVar(*this, IntSet(finishWarehouseFields,12)));
            }
          }

          */




          // SYMMETRIES

          // Null Jobs at the end.
          // TODO: By adding more robots, this must be modified
          if (i < maxTasks-1) {
           ite(*this, robotTasksBool(i,0), IntVar(*this, 0, 0), IntVar(*this, 0, 4), robotTasks[i+1]);
          }
          //if (i > 0) {
            //  ite(*this, robotTasksBool(i,0), IntVar(*this, 0, 0), IntVar(*this, 0, 4), robotTasks[i-1]);
          //}

      }




      /// COST

      // TODO: Modify the cost function for priority and other task constraints

      /// Mapping Cost of a job to each task of the robot
      for (int i = 0; i < maxTasks; i++) {
          element(*this, jobCost, robotTasks[i], robotTasksCost[i]);
      }
      /// linear all RobotTaskCost to the complete cost.
      linear(*this, robotTasksCost, IRT_EQ, robotCost);



      for (int i = 0; i < __numGoods; i++) {
          for (int j = 0; j < __numWarehouses; j++) {
              int posWarehouse = __warehousePosition[j];
              BoolVar goodAtWarehouse = expr(*this, goodsPositionEnd(maxTasks-1,i) == IntVar(*this, posWarehouse, posWarehouse));
              bool goodFulfillHelp = __goodsTempMin[i] <= __warehouseTemp[j] && __warehouseTemp[j] <= __goodsTempMax[i]
                && __goodsLightMin[i] <= __warehouseLight[j] && __warehouseLight[j] <= __goodsLightMax[i];
              BoolVar goodFulfill(*this, goodFulfillHelp, goodFulfillHelp);

              ite(*this, expr(*this, goodAtWarehouse && goodFulfill), IntVar(*this, 0, 0), IntVar(*this, 0, 100), goodsPenaltyCost[i]);
              ite(*this, expr(*this, goodAtWarehouse && !(goodFulfill)), IntVar(*this, 100, 100), IntVar(*this, 0, 100), goodsPenaltyCost[i]);


          }


          for (int j = 0; j < __numGarage; j++) {
              int posGarage = __garagePosition[j];
              ite(*this, expr(*this, goodsPositionEnd(maxTasks-1,i) == IntVar(*this, posGarage, posGarage)),
                IntVar(*this, 100, 100), IntVar(*this, 0, 100), goodsPenaltyCost[i]);
          }
      }



      /// linear all RobotTaskCost to the complete cost.
      linear(*this, goodsPenaltyCost, IRT_EQ, penaltyCost);

      /// Total cost will be robotCost + goodsPenaltyCost
      rel(*this, robotCost + penaltyCost == c);


      /// BRANCHING

      // First branch on the cost, so that we get every-time the minimum cost.
      //branch(*this, c, INT_VAL_MIN());

      // Branch then on the different tasks.
      //branch(*this, robotTasks, INT_VAR_SIZE_MIN(), INT_VAL_MIN());

      branch(*this, robotTasks, INT_VAR_AFC_SIZE_MIN(), INT_VAL_MIN());

      // Branch then on the number of moving steps for the case of a
      // moving task.
      branch(*this, robotMovingForward, INT_VAR_AFC_SIZE_MIN(), INT_VAL_MIN());

      // Branch then on the orientation, left or right turn for the case
      // of a turning task.
      branch(*this, robotOrientDiff, INT_VAR_AFC_SIZE_MIN(), INT_VAL_RND(5));

  }




  /// Constructor for cloning \a s
  Warehouse(bool share, Warehouse& s) : IntMinimizeScript(share,s) {
    robotTasks.update(*this, share, s.robotTasks);
    robotTasksBoolArray.update(*this, share, s.robotTasksBoolArray);
    robotTasksCost.update(*this, share, s.robotTasks);
    robotCost.update(*this, share, s.robotCost);
    robotPositionsStart.update(*this, share, s.robotPositionsStart);
    robotPositionsStartBoolArray.update(*this, share, s.robotPositionsStartBoolArray);
    robotPositionsEnd.update(*this, share, s.robotPositionsEnd);
    robotPositionsDiff.update(*this, share, s.robotPositionsDiff);
    robotPositionsDiffTemp.update(*this, share, s.robotPositionsDiffTemp);
    robotMovingForward.update(*this, share, s.robotMovingForward);
    robotOrientationStart.update(*this, share, s.robotOrientationStart);
    robotOrientationStartBoolArray.update(*this, share, s.robotOrientationStartBoolArray);
    robotOrientationEnd.update(*this, share, s.robotOrientationEnd);
    robotOrientDiff.update(*this, share, s.robotOrientDiff);
    robotOrientMod.update(*this, share, s.robotOrientMod);
    robotGoodsStart.update(*this, share, s.robotGoodsStart);
    robotGoodsEnd.update(*this, share, s.robotGoodsEnd);
    goodsPositionStartArray.update(*this, share, s.goodsPositionStartArray);
    goodsPositionEndArray.update(*this, share, s.goodsPositionEndArray);
    goodsPenaltyCost.update(*this, share, s.goodsPenaltyCost);
    penaltyCost.update(*this, share, s.penaltyCost);
    c.update(*this, share, s.c);
  }
  /// Copy during cloning
  virtual Space*
  copy(bool share) {
    return new Warehouse(share,*this);
  }
  virtual IntVar cost(void) const{
    return c;
  }

  /// Print solution
  virtual void
  print(std::ostream& os) const {
    // Additional printing


    //os << "\t" << robotTasks << std::endl;

    __foundSolution = true;

    __final_output = "";
    __final_output += "INSTRUCTIONS:";

    // Save the current task and the previous task for some robot constraints.
    // The robot should not get the instruction forward 1 and backward 1, when
    // there is a picking up or a dropping task in the middle. This will be
    // at the output only one task.
    string temp = "";
    string tempPrevious = "";
    bool printPrevious = false;
    bool printCurrent = false;
    bool afterBackward = __robotLastBackwardBefore;

    for (int i = 0; i < maxTasks; i++) {

        // Copy old current task to the previous task.
        tempPrevious = temp;

        // If the current task is a turning task, then the previous task and
        // the current task should be printed.
        if (robotTasks[i].val() == 1) {
          temp = "TURN,";
          // Checking After Backward or not
          if (afterBackward) {
              temp += "2,";
          } else {
              temp += "1,";
          }
          // Checking if left or right
          if (robotOrientDiff[i].val() == -1) {
              temp += "LEFT";
          } else if (robotOrientDiff[i].val() == 1) {
              temp += "RIGHT";
          } else {
            // TODO: throw exception
          }
          temp += ";";
          printPrevious = printCurrent && true;
          printCurrent = true;
          afterBackward = false;

          // TODO: After a backward task, there should be a 2 instead of a 1.

        }
        else if (robotTasks[i].val() == 2) {

          // If the current task is a backward task, then this task should not
          // be printed, but the previous task can be printed
          if (robotMovingForward[i].val() == -1) {
            temp = "BACKWARD,1;";
            printPrevious = printCurrent && true;
            printCurrent = false;
            afterBackward = true;
          } else {

            // If the current task is a normal forward task, then the current
            // and the previous task can be printed. The number of forward
            // steps (1 to 6) is printed, too.
            stringstream stringStreamForward;
            stringStreamForward << robotMovingForward[i].val();
            temp = "FORWARD," + stringStreamForward.str() + ",";
            // Adding which Sensors
            if (robotOrientationStart[i].val() == 0 || robotOrientationStart[i].val() == 1) {
                temp += "0"; // Right sensor
            } else if (robotOrientationStart[i].val() == 2 || robotOrientationStart[i].val() == 3) {
                temp += "1"; // Left sensor
            } else {
                // TODO: throw exception
            }
            temp += ";";
            printPrevious = printCurrent && true;
            printCurrent = true;
            afterBackward = false;
          }
        }
        // If the current task is a picking task, then this task can be printed,
        // but not the previous forward 1 task.
        else if (robotTasks[i].val() == 3) {
          temp = "PICK,1;";
          printPrevious = printCurrent && false;
          printCurrent = true;
          afterBackward = false;
        }
        // If the current task is a dropping task, then this task can be
        // printed, but not the previous forward 1 task.
        else if (robotTasks[i].val() == 4) {
          temp = "DROP,1;";
          printPrevious = printCurrent && false;
          printCurrent = true;
          afterBackward = false;
        }
        // Otherwise: Dont print only the previous one, but not the current
        // task.
        else {
          temp = "";
          printPrevious = printCurrent && true;
          printCurrent = false;
          if (robotTasks[i].val() != 0) {
              afterBackward = false;
          }
        }
        if (printPrevious) {
          __final_output +=  tempPrevious;
        }
    }
    // Print at the end also the current task, if the current task should be
    // printed.
    if (printCurrent) {
      __final_output += temp;
    }

    __robotLastBackwardAfter = afterBackward;

    __robotFinalPosition = robotPositionsEnd[maxTasks-1].val();
    __robotFinalOrientation = robotOrientationEnd[maxTasks-1].val();

    IntArgs goodsEndPositions(__numGoods);

    Matrix<IntVarArray> goodsPositionEnd(goodsPositionEndArray,maxTasks,__numGoods);

    for (int j = 0; j < __numGoods; j++) {
        goodsEndPositions[j] = goodsPositionEnd(maxTasks-1,j).val();
    }

    __goodsEndPositions = goodsEndPositions;

    // Additional printing:
    //os << "\t" << robotPositionsStart << std::endl;
    //os << "\t" << robotPositionsEnd << std::endl;
    //os << "\t" << robotMovingForward << std::endl;
    //os << "\t" << robotOrientationStart << std::endl;
    //os << "\t" << robotOrientationEnd << std::endl;
    //os << "\t" << robotGoodsStart << std::endl;
    //os << "\t" << robotGoodsEnd << std::endl;
    //os << "\t" << goodsPositionStartArray << std::endl;
    //os << "\t" << goodsPositionStartArray << std::endl;
    //os << "\t" << c << std::endl;
  }

};




/* -------------------------------
 *  MODIFYING THE SCRIPT FOR RUNNING
 *  AND ADDITIONAL PRINTING
 *  -------------------------------
 */
template<class BaseSpace>
class ScriptBaseCustom : public BaseSpace {
public:
    /// Default constructor
    ScriptBaseCustom(void) {}
    /// Constructor used for cloning
    ScriptBaseCustom(bool share, ScriptBaseCustom& e) : BaseSpace(share,e) {}
    /// Print a solution to \a os
    virtual void print(std::ostream& os) const { (void) os; }
    /// Compare with \a s
    virtual void compare(const Space&, std::ostream& os) const {
        (void) os;
    }
    /// Choose output stream according to \a name
    static std::ostream& select_ostream(const char* name, std::ofstream& ofs);

    template<class Script, template<class> class Engine, class Options>
    static void run(const Options& opt, Script* s=NULL);
private:
    template<class Script, template<class> class Engine, class Options,
    template<template<class> class,class> class Meta>
    static void runMeta(const Options& opt, Script* s);
    /// Catch wrong definitions of copy constructor
    explicit ScriptBaseCustom(ScriptBaseCustom& e);
};

template<class Space>
std::ostream&
ScriptBaseCustom<Space>::select_ostream(const char* name, std::ofstream& ofs) {
    if (strcmp(name, "stdout") == 0) {
        return std::cout;
    } else if (strcmp(name, "stdlog") == 0) {
        return std::clog;
    } else if (strcmp(name, "stderr") == 0) {
        return std::cerr;
    } else {
        ofs.open(name);
        return ofs;
    }
}

template<class Space>
template<class Script, template<class> class Engine, class Options>
void
ScriptBaseCustom<Space>::run(const Options& o, Script* s) {
    if (o.restart()==RM_NONE) {
        runMeta<Script,Engine,Options,EngineToMeta>(o,s);
    } else {
        runMeta<Script,Engine,Options,RBS>(o,s);
    }
}

template<class Space>
template<class Script, template<class> class Engine, class Options,
template<template<class> class,class> class Meta>
void
ScriptBaseCustom<Space>::runMeta(const Options& o, Script* s) {
    using namespace std;

    ofstream sol_file, log_file;

    ostream& s_out = select_ostream(o.out_file(), sol_file);
    ostream& l_out = select_ostream(o.log_file(), log_file);

    try {
        switch (o.mode()) {
            case SM_SOLUTION:
            {
                Support::Timer t;
                int i = o.solutions();
                t.start();
                if (s == NULL)
                    s = new Script(o);
                Search::Options so;
                so.threads = o.threads();
                so.c_d     = o.c_d();
                so.a_d     = o.a_d();
                so.stop    = CombinedStop::create(o.node(),o.fail(), o.time(),
                                                  o.interrupt());
                so.cutoff  = createCutoff(o);
                so.clone   = false;
                if (o.interrupt())
                    CombinedStop::installCtrlHandler(true);
                {
                    Meta<Engine,Script> e(s,so);
                    if (o.print_last()) {
                        Script* px = NULL;
                        do {
                            Script* ex = e.next();
                            if (ex == NULL) {
                                if (px != NULL) {
                                    px->print(s_out);
                                    delete px;
                                }
                                break;
                            } else {
                                delete px;
                                px = ex;
                            }
                        } while (--i != 0);
                    } else {
                        do {
                            Script* ex = e.next();
                            if (ex == NULL)
                                break;
                            ex->print(s_out);
                            delete ex;
                        } while (--i != 0);
                    }
                    if (o.interrupt())
                        CombinedStop::installCtrlHandler(false);
                    Search::Statistics stat = e.statistics();
                    if (e.stopped()) {
                        int r = static_cast<CombinedStop*>(so.stop)->reason(stat,so);
                        if (r & CombinedStop::SR_INT){
                          //l_out << "user interrupt " << endl;
                        }
                        else {
                            if (r & CombinedStop::SR_NODE) {
                                //l_out << "node ";
                              }
                            if (r & CombinedStop::SR_FAIL) {
                                //l_out << "fail ";
                              }
                            if (r & CombinedStop::SR_TIME) {
                                //l_out << "time ";
                            //l_out << "$>\\Timeout$";
                          }
                        }
                    }
                    else {
                        double runtime_msec = t.stop();
                        double runtime_sec = runtime_msec/1000;
                        //l_out.width(8);
                        //l_out << showpoint << fixed
                        //<< setprecision(3) << runtime_sec ;
                    }
                    //cout
                    //<< "\t&\t"
                    //<< stat.fail
                    // << "\t \\\\";
                    //<< endl;
                }
                delete so.stop;
            }
                break;
            default:
                Script::template run<Warehouse,DFS,Options>(o);
        }
    } catch (Exception& e) {
        cerr << "Exception: " << e.what() << "." << endl
        << "Stopping..." << endl;
        if (sol_file.is_open())
            sol_file.close();
        if (log_file.is_open())
            log_file.close();
        exit(EXIT_FAILURE);
    }
    if (sol_file.is_open())
        sol_file.close();
    if (log_file.is_open())
        log_file.close();
}

typedef ScriptBaseCustom<Space> ScriptOutput;





void split(const string& s, char c,
           vector<string>& v) {
   string::size_type i = 0;
   string::size_type j = s.find(c);

   while (j != string::npos) {
      v.push_back(s.substr(i, j-i));
      i = ++j;
      j = s.find(c, j);

      if (j == string::npos)
         v.push_back(s.substr(i, s.length()));
   }
}




/** \brief Main-function
 *  \relates Warehouse
 */
int
main(int argc, char* argv[]) {

    /// TODO: reading/parsing json files
    /// Test Initializing

        // Readin stdin

            Json::Value job_root;
            std::cin >> job_root;

            string job_kind = job_root.get("job", "null").asString();

            if (job_kind == "moving") {
                __robotBoolMoving = true;
                int robot_destination_x = job_root["to"]["x_coord"].asInt();
                int robot_destination_y = job_root["to"]["y_coord"].asInt();
                __robotBoolMovingPos = robot_destination_y * 7 + robot_destination_x;

                //cout << __robotBoolMovingPos << "\n";

            }
            else if (job_kind == "placeGood") {
                __robotBoolPlaceGood = true;

                int robot_from_x = job_root["from"]["x_coord"].asInt();
                int robot_from_y = job_root["from"]["y_coord"].asInt();
                __robotBoolPlaceGoodFromPos = robot_from_y * 7 + robot_from_x;



                int robot_to_x = job_root["to"]["x_coord"].asInt();
                int robot_to_y = job_root["to"]["y_coord"].asInt();
                __robotBoolPlaceGoodToPos = robot_to_y * 7 + robot_to_x;

            }
            else if (job_kind == "add") {

                __robotBoolAddGood = true;

                // Pos (1,1) = 8

            }
            else if (job_kind == "drop") {

                __robotBoolDropGood = true;

                int robot_from_x = job_root["from"]["x_coord"].asInt();
                int robot_from_y = job_root["from"]["y_coord"].asInt();
                __robotBoolDropGoodFromPos = robot_from_y * 7 + robot_from_x;

                // To Pos (1,2) = 15



            }
            else {
                __robotBoolMoving = false;
                __robotBoolPlaceGood = false;
                __robotBoolAddGood = false;
                __robotBoolDropGood = false;
            }






        /// Parse Json Files
        Json::Value json_robot_root;   // starts as "null"; will contain the root value after parsing
        std::fstream config_robot("robot.js");

        config_robot >> json_robot_root;

        config_robot.close();

        // Get the value of the member of root named 'my-encoding', return 'UTF-32' if there is no
        // such member.
        //std::string my_encoding = root.get("my-encoding", "UTF-32" ).asString();

        // Get the value of the member of root named 'my-plug-ins'; return a 'null' value if
        // there is no such member.
        int robot_start_x = json_robot_root["x_coord"].asInt();
        int robot_start_y = json_robot_root["y_coord"].asInt();

        int robot_start_position = robot_start_y * 7 + robot_start_x;

        int robot_start_orientation = json_robot_root["orientation"].asInt();

        bool robot_last_backward = json_robot_root["backward"].asBool();

        __robotStartPosition = robot_start_position;
        __robotStartOrientation = robot_start_orientation;
        __robotLastBackwardBefore = robot_last_backward;

        /*

        cout << "robot start row: " << robot_start_x << "\n";
        cout << "robot start column: " << robot_start_y << "\n";
        cout << "robot start position: " << robot_start_position << "\n";
        cout << "robot start orientation: " << robot_start_orientation << "\n";

        */

        // Sensors

        Json::Value json_sensors_root;
        std::ifstream config_sensors("sensors.js");
        config_sensors >> json_sensors_root;

        IntArgs sensorsTemperature(json_sensors_root.size());
        IntArgs sensorsLight(json_sensors_root.size());

        for ( int i = 0; i < json_sensors_root.size(); ++i ){
            sensorsTemperature[i] = json_sensors_root[i]["temperature"].asInt();
            sensorsLight[i] = json_sensors_root[i]["lighting"].asInt();

        }

        //cout << sensorsTemperature << "\n";
        //cout << sensorsLight << "\n";

        // Sections and Areas

        Json::Value json_sections_root;
        std::ifstream config_sections("sections.js");
        config_sections >> json_sections_root;

        int numGoods = 0;

        int numWarehouses = 0;
        int numGarage = 0;

        for ( int i = 0; i < json_sections_root.size(); ++i ){
            int sensor_int = json_sections_root[i]["sensor"].asInt();
            if (sensor_int > 0) {
                numWarehouses++;
            } else {
                numGarage++;
            }
            string good_bool = json_sections_root[i]["status"].asString();
            if (good_bool == "occupied") {
                numGoods++;
            }
        }

        /*

        cout << numGoods << "\n";
        cout << numWarehouses << "\n";
        cout << numGarage << "\n";

        */

        if (__robotBoolAddGood) {
            __numGoods = numGoods + 1;
        } else {
            __numGoods = numGoods;
        }

        if (__numGoods == 0) {
            // Create Dummy Good
            __boolDummyGood = true;
            __robotBoolDropGood = true;
            __robotBoolDropGoodNumber = 0;
            __robotBoolDropGoodFromPos = 15;
            __numGoods = 1;
        }

        __numWarehouses = numWarehouses;
        __numGarage = numGarage;


        IntArgs goodsStartingPosition(__numGoods);

        IntArgs goodsTempMin(__numGoods);
        IntArgs goodsTempMax(__numGoods);

        IntArgs goodsLightMin(__numGoods);
        IntArgs goodsLightMax(__numGoods);

        vector<string> goodsName;

        IntArgs warehouseTemp(numWarehouses);
        IntArgs warehouseLight(numWarehouses);
        IntArgs warehousePosition(__numWarehouses);

        IntArgs garagePosition(__numGarage);

        int it_numGoods = 0;

        int it_numWarehouses = 0;
        int it_numGarage = 0;

        for ( int i = 0; i < json_sections_root.size(); ++i ){
            int cur_x = json_sections_root[i]["x_coord"].asInt();
            int cur_y = json_sections_root[i]["y_coord"].asInt();
            int cur_pos = cur_y * 7 + cur_x;

            int sensor_int = json_sections_root[i]["sensor"].asInt();
            if (sensor_int > 0) {
                warehousePosition[it_numWarehouses] = cur_pos;
                warehouseTemp[it_numWarehouses] = sensorsTemperature[sensor_int-1];
                warehouseLight[it_numWarehouses] = sensorsLight[sensor_int-1];
                it_numWarehouses++;
            } else {
                garagePosition[it_numGarage] = cur_pos;
                it_numGarage++;
            }
            string good_bool = json_sections_root[i]["status"].asString();
            if (good_bool == "occupied") {
                goodsStartingPosition[it_numGoods] = cur_pos;
                goodsName.push_back(json_sections_root[i]["good"]["name"].asString());
                goodsTempMin[it_numGoods] = json_sections_root[i]["good"]["desiredTemperature"]["min"].asInt();
                goodsTempMax[it_numGoods] = json_sections_root[i]["good"]["desiredTemperature"]["max"].asInt();
                goodsLightMin[it_numGoods] = json_sections_root[i]["good"]["desiredLighting"]["min"].asInt();
                goodsLightMax[it_numGoods] = json_sections_root[i]["good"]["desiredLighting"]["max"].asInt();
                it_numGoods++;
            }
        }

        if (__robotBoolAddGood) {
            goodsStartingPosition[__numGoods-1] = 8; // Starting Position for Adding Goods
            goodsName.push_back(job_root["good"]["name"].asString());
            goodsTempMin[__numGoods-1] = job_root["good"]["desiredTemperature"]["min"].asInt();
            goodsTempMax[__numGoods-1] = job_root["good"]["desiredTemperature"]["max"].asInt();
            goodsLightMin[__numGoods-1] = job_root["good"]["desiredLighting"]["min"].asInt();
            goodsLightMax[__numGoods-1] = job_root["good"]["desiredLighting"]["max"].asInt();
        }

        if (__boolDummyGood) {

            goodsStartingPosition[__numGoods-1] = 15; // Starting Position for Dropping Goods
            goodsName.push_back("DummyGood");
            goodsTempMin[__numGoods-1] = -100;
            goodsTempMax[__numGoods-1] = 2000;
            goodsLightMin[__numGoods-1] = -1000;
            goodsLightMax[__numGoods-1] = 2000;

        }

        __goodsStartingPosition = goodsStartingPosition;
        __goodsTempMin = goodsTempMin;
        __goodsTempMax = goodsTempMax;
        __goodsLightMin = goodsLightMin;
        __goodsLightMax = goodsLightMax;

        __warehouseTemp = warehouseTemp;
        __warehouseLight = warehouseLight;
        __warehousePosition = warehousePosition;
        __garagePosition = garagePosition;

        /*
        cout << __goodsStartingPosition << "\n";
        cout << __goodsTempMin << "\n";
        cout << __goodsTempMax << "\n";
        cout << __goodsLightMin << "\n";
        cout << __goodsLightMax << "\n";
        cout << __warehouseTemp << "\n";
        cout << __warehouseLight << "\n";
        cout << __warehousePosition << "\n";
        cout << __garagePosition << "\n";
        */

        if (job_kind == "placeGood") {
            bool checkFindGood = false;

            for (int j = 0; j < __numGoods; j++) {
                if (__goodsStartingPosition[j] == __robotBoolPlaceGoodFromPos) {
                    __robotBoolPlaceGoodNumber = j;
                    //cout << __robotBoolPlaceGoodNumber << "\n";
                    checkFindGood = true;
                }
            }

            if (!(checkFindGood)) {
                __robotBoolPlaceGood = false;
            }
        } else if (job_kind == "drop") {

            bool checkFindGood = false;

            for (int j = 0; j < __numGoods; j++) {
                if (__goodsStartingPosition[j] == __robotBoolDropGoodFromPos) {
                    __robotBoolDropGoodNumber = j;
                    checkFindGood = true;
                }
            }

            if (!(checkFindGood)) {
                __robotBoolDropGood = false;
            }
        }



        /// TODO: Initializing the IntArgs as global variables


/*
    /// Number of goods.
    int numGoods = 2;
    __numGoods = numGoods;

    /// Start position of all goods
    IntArgs goodsStartingPosition(__numGoods);
    goodsStartingPosition[0] = 9;
    goodsStartingPosition[1] = 8;
    __goodsStartingPosition = goodsStartingPosition;


    /// Minimum and Maximum Temperatur of goods.
    IntArgs goodsTempMin(__numGoods);
    goodsTempMin[0] = 10;
    goodsTempMin[1] = 10;
    __goodsTempMin = goodsTempMin;

    IntArgs goodsTempMax(__numGoods);
    goodsTempMax[0] = 10;
    goodsTempMax[1] = 100;
    __goodsTempMax = goodsTempMax;

    /// Minimum and Maximum Light of goods.
    IntArgs goodsLightMin(__numGoods);
    goodsLightMin[0] = 1;
    goodsLightMin[1] = 1;
    __goodsLightMin = goodsLightMin;

    IntArgs goodsLightMax(__numGoods);
    goodsLightMax[0] = 100;
    goodsLightMax[1] = 100;
    __goodsLightMax = goodsLightMax;


    /// Number of Warehouse places.
    int numWarehouses = 12;
    __numWarehouses = numWarehouses;

    /// Temperatur of Warehouses.
    IntArgs warehouseTemp(numWarehouses);
    warehouseTemp[0] = 10;
    warehouseTemp[1] = 20;
    warehouseTemp[2] = 20;
    warehouseTemp[3] = 20;
    warehouseTemp[4] = 20;
    warehouseTemp[5] = 20;
    warehouseTemp[6] = 20;
    warehouseTemp[7] = 20;
    warehouseTemp[8] = 20;
    warehouseTemp[9] = 20;
    warehouseTemp[10] = 20;
    warehouseTemp[11] = 20;
    __warehouseTemp = warehouseTemp;

    /// Light of Warehouses.
    IntArgs warehouseLight(numWarehouses);
    warehouseLight[0] = 10;
    warehouseLight[1] = 12;
    warehouseLight[2] = 18;
    warehouseLight[3] = 20;
    warehouseLight[4] = 10;
    warehouseLight[5] = 10;
    warehouseLight[6] = 10;
    warehouseLight[7] = 20;
    warehouseLight[8] = 14;
    warehouseLight[9] = 10;
    warehouseLight[10] = 20;
    warehouseLight[11] = 20;
    __warehouseLight = warehouseLight;

    /// Position of the Warehouse
    IntArgs warehousePosition(__numWarehouses);
    warehousePosition[0] = 11;
    warehousePosition[1] = 12;
    warehousePosition[2] = 18;
    warehousePosition[3] = 19;
    warehousePosition[4] = 29;
    warehousePosition[5] = 30;
    warehousePosition[6] = 32;
    warehousePosition[7] = 33;
    warehousePosition[8] = 36;
    warehousePosition[9] = 37;
    warehousePosition[10] = 39;
    warehousePosition[11] = 40;
    __warehousePosition = warehousePosition;


    /// Number of garage places.
    int numGarage = 4;
    __numGarage = numGarage;

    /// Position of the garage places
    IntArgs garagePosition(__numGarage);
    garagePosition[0] = 8;
    garagePosition[1] = 9;
    garagePosition[2] = 15;
    garagePosition[3] = 16;
    __garagePosition = garagePosition;

*/

    //cout << "Test" << endl;

    /// Running the script with a dept-first-search
    Options opt("");
    opt.solutions(0);
    opt.parse(argc,argv);
    ScriptOutput::run<Warehouse,BAB,Options>(opt);

    if (__foundSolution) {

            cout << __final_output << endl;

            // Updating files

            // Updating Robot

            json_robot_root["x_coord"] = __robotFinalPosition % 7;
            json_robot_root["y_coord"] = __robotFinalPosition / 7;
            json_robot_root["orientation"] = __robotFinalOrientation;
            json_robot_root["backward"] = __robotLastBackwardAfter;

            Json::StyledWriter styledWriter;
            std::ofstream ofs;
            ofs.open("robot.js", std::ofstream::out | std::ofstream::trunc);
            ofs << styledWriter.write(json_robot_root);
            ofs.close();

            // Updating Sections


            for ( int i = 0; i < json_sections_root.size(); ++i ){
                int cur_x = json_sections_root[i]["x_coord"].asInt();
                int cur_y = json_sections_root[i]["y_coord"].asInt();
                int cur_pos = cur_y * 7 + cur_x;

                bool isOccupied = false;


                for (int j = 0; j < __numGoods; j++) {
                    if (__robotBoolDropGood) {
                        if (__goodsEndPositions[j] == cur_pos && j != __robotBoolDropGoodNumber) {
                            isOccupied = true;
                            json_sections_root[i]["status"] = "occupied";
                            Json::Value good;
                            good["name"] = goodsName[j].c_str();
                            Json::Value good_temp;
                            good_temp["min"] = __goodsTempMin[j];
                            good_temp["max"] = __goodsTempMax[j];
                            good["desiredTemperature"] = good_temp;
                            Json::Value good_light;
                            good_light["min"] = __goodsLightMin[j];
                            good_light["max"] = __goodsLightMax[j];
                            good["desiredLighting"] = good_light;
                            json_sections_root[i]["good"] = good;
                        }
                    } else {
                        if (__goodsEndPositions[j] == cur_pos) {
                            isOccupied = true;
                            json_sections_root[i]["status"] = "occupied";
                            Json::Value good;
                            good["name"] = goodsName[j].c_str();
                            Json::Value good_temp;
                            good_temp["min"] = __goodsTempMin[j];
                            good_temp["max"] = __goodsTempMax[j];
                            good["desiredTemperature"] = good_temp;
                            Json::Value good_light;
                            good_light["min"] = __goodsLightMin[j];
                            good_light["max"] = __goodsLightMax[j];
                            good["desiredLighting"] = good_light;
                            json_sections_root[i]["good"] = good;
                        }
                    }

                }


                if (!(isOccupied)) {
                    json_sections_root[i]["status"] = "free";
                    json_sections_root[i]["good"] = false;
                }

            }

            Json::StyledWriter styledWriterSections;
            std::ofstream ofsSections;
            ofsSections.open("sections.js", std::ofstream::out | std::ofstream::trunc);
            ofsSections << styledWriterSections.write(json_sections_root);
            ofsSections.close();



    } else {

        cout << "INSTRUCTIONS:" << endl;

    }



    return 0;
}
