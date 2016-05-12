/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main author:
 *     Michael Brendle <michaelbrendle@me.com>
 *
 */

#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

using namespace std;
using namespace Gecode;
using namespace Gecode::Driver;



class Warehouse : public Script {
protected:

  /// Number of maximal robot tasks.
  static const int maxTasks = 18;
  /// Array of robotTasks and the corresponding BoolArray.
  IntVarArray robotTasks;
  BoolVarArray robotTasksBoolArray;

  /// Array of the cost for each task.
  IntVarArray robotTasksCost;

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


  /// Number of goods.
  static const int numGoods = 1;

  /// Matrix/Array of the positions of the goods at start and at end.
  IntVarArray goodsPositionStartArray;
  IntVarArray goodsPositionEndArray;


  // TODO: Adding constraints for temperature and light.
  /*
  /// Minimum and Maximum Temperatur of goods.
  IntArgs tempMinGoods;
  IntArgs tempMaxGoods;
  /// Minimum and Maximum Light of goods.
  IntArgs lightMinGoods;
  IntArgs lightMaxGoods;

  /// Number of Warehouse places.
  static const int numWarehouses = 12;
  /// Temperatur of Warehouses.
  IntArgs tempWarehouses;
  /// Light of Warehouses.
  IntArgs lightWarehouses;

  /// Number of garage places.
  static const int numGarage = 4;
  */

  /// Total cost (want to minimize).
  IntVar c;

public:
  /// Actual model
  Warehouse(const Options& opt) : Script(opt),
  robotTasks(*this,maxTasks,0,4),
  robotTasksBoolArray(*this,maxTasks*5,0,1),
  robotTasksCost(*this,maxTasks,0,500),
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
  robotGoodsStart(*this,maxTasks,-1,numGoods-1), // -1 no goods
  robotGoodsEnd(*this,maxTasks,-1,numGoods-1), // -1 no goods
  goodsPositionStartArray(*this,maxTasks*numGoods,0,48),
  goodsPositionEndArray(*this,maxTasks*numGoods,0,48),
  c(*this,0,1000)
    {




      /// SETTING UP MATRIZES

      // Setting up the two Matrizes for the positions of the goods at the
      // start and at the end of the task.
      Matrix<IntVarArray> goodsPositionStart(goodsPositionStartArray,maxTasks,numGoods);
      Matrix<IntVarArray> goodsPositionEnd(goodsPositionEndArray,maxTasks,numGoods);

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

      // Help Array for the 33 street fields.
      int streetFields [33] = {0,1,2,3,4,5,6,7,10,13,14,17,20,21,22,23,24,25,26,27,28,31,34,35,38,41,42,43,44,45,46,47,48};




      /// START CONSTRAINTS

      // TODO: Will be read later from the database

      // Start position and orientation of the robot
      rel(*this, robotPositionsStart[0] == 0);
      rel(*this, robotOrientationStart[0] == 1); // 1 = East

      // The current good at the robot; -1 means that no good is at the robot;
      // Otherwise 0 to numGoods-1
      rel(*this, robotGoodsStart[0] == -1);

      // The starting position from good number 0. Here position 9 in the
      // Warehouse.
      rel(*this, goodsPositionStart(0,0) == 9);




      /// CONSTRAINTS FOR EACH TASK

      for (int i = 0; i < maxTasks; i++) {

          /// END - START CONSTRAINTS

          if (i < maxTasks - 1) {

            // End Position of task i is the start position of task i + 1
            // This holds for the robot and for the goods.
            rel(*this, robotPositionsEnd[i] == robotPositionsStart[i+1]);
            rel(*this, robotOrientationEnd[i] == robotOrientationStart[i+1]);
            for (int j = 0; j < numGoods; j++) {
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
          for (int j=0; j < numGoods; j++) {
            ite(*this, expr(*this, !(robotTasksBool(i,2) && (robotGoodsStart[i] == IntVar(*this, j, j)))), goodsPositionStart(i,j), robotPositionsEnd[i], goodsPositionEnd(i,j));
          }




          /// TASK 3: PICKING UP

          // Help IntVars:
          IntVar definedRobotGoodsEnd(*this, 0, numGoods - 1);
          IntVar definedRobotPositionsEnd(*this, 0, 48);

          // If the current task is a picking up task, then the robot must
          // have no goods at the start of this task. Otherwise, the
          // robot could have goods or not.
          ite(*this, robotTasksBool(i,3), IntVar(*this,-1,-1), IntVar(*this,-1,numGoods-1), robotGoodsStart[i]);

          // If the current task is a picking up task, then the good of the
          // robot is stored in the help IntVar definedRobotGoodsEnd.
          ite(*this, robotTasksBool(i,3), definedRobotGoodsEnd, IntVar(*this, -1, numGoods-1), robotGoodsEnd[i]);

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
          ite(*this, robotTasksBool(i,4), IntVar(*this,-1,-1), IntVar(*this,-1,numGoods-1), robotGoodsEnd[i]);

          // If the current task is a dropping task, then the robot must have
          // a good at the start of the task. Otherwise, it can be anything.
          ite(*this, robotTasksBool(i,4), IntVar(*this,0,numGoods-1), IntVar(*this,-1,numGoods-1), robotGoodsStart[i]);




          /// TASK 3 OR 4: PICKING OR DROPPING

          // If the current task is not a picking or a dropping task, then
          // the good at the robot will be the same before and after the task.
          ite(*this, expr(*this, robotTasksBool(i,3) || robotTasksBool(i,4)), IntVar(*this, -1, numGoods-1), robotGoodsStart[i], robotGoodsEnd[i]);


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

          // Garage Warehouse Places must be empty at the end -> No Goods Inside
          if (i == maxTasks-1) {
            for (int j = 0; j < numGoods; j++) {
                rel(*this, goodsPositionEnd(i,j) == IntVar(*this, IntSet(finishWarehouseFields,12)));
            }
          }




          // SYMMETRIES

          // Null Jobs at the end.
          // TODO: By adding more robots, this must be modified
          if (i < maxTasks-1) {
              ite(*this, robotTasksBool(i,0), IntVar(*this, 0, 0), IntVar(*this, 0, 4), robotTasks[i+1]);
          }

      }




      /// COST

      // TODO: Modify the cost function for priority and other task constraints

      /// Mapping Cost of a job to each task of the robot
      for (int i = 0; i < maxTasks; i++) {
          element(*this, jobCost, robotTasks[i], robotTasksCost[i]);
      }

      /// linear all RobotTaskCost to the complete cost.
      linear(*this, robotTasksCost, IRT_EQ, c);




      /// BRANCHING

      // First branch on the cost, so that we get every-time the minimum cost.
      branch(*this, c, INT_VAL_MIN());

      // Branch then on the different tasks.
      branch(*this, robotTasks, INT_VAR_AFC_SIZE_MIN(), INT_VAL_RND(5));

      // Branch then on the number of moving steps for the case of a
      // moving task.
      branch(*this, robotMovingForward, INT_VAR_AFC_SIZE_MIN(), INT_VAL_MIN());

      // Branch then on the orientation, left or right turn for the case
      // of a turning task.
      branch(*this, robotOrientDiff, INT_VAR_AFC_SIZE_MIN(), INT_VAL_RND(5));

  }




  /// Constructor for cloning \a s
  Warehouse(bool share, Warehouse& s) : Script(share,s) {
    robotTasks.update(*this, share, s.robotTasks);
    robotTasksBoolArray.update(*this, share, s.robotTasksBoolArray);
    robotTasksCost.update(*this, share, s.robotTasks);
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
    c.update(*this, share, s.c);
  }
  /// Copy during cloning
  virtual Space*
  copy(bool share) {
    return new Warehouse(share,*this);
  }

  /// Print solution
  virtual void
  print(std::ostream& os) const {
    // Additional printing
    //os << "\t" << robotTasks << std::endl;

    os << "INSTRUCTIONS:";

    // Save the current task and the previous task for some robot constraints.
    // The robot should not get the instruction forward 1 and backward 1, when
    // there is a picking up or a dropping task in the middle. This will be
    // at the output only one task.
    string temp = "";
    string tempPrevious = "";
    bool printPrevious = false;
    bool printCurrent = false;

    for (int i = 0; i < maxTasks; i++) {

        // Copy old current task to the previous task.
        tempPrevious = temp;

        // If the current task is a turning task, then the previous task and
        // the current task should be printed.
        if (robotTasks[i].val() == 1) {
          temp = "TURN,1,";
          // Checking if left or right
          if (robotOrientDiff[i].val() == -1) {
              temp += "LEFT";
          } else if (robotOrientDiff[i].val() == 1) {
              temp += "RIGHT";
          } else {
            // TODO: throw excpetion
          }
          temp += ";";
          printPrevious = printCurrent && true;
          printCurrent = true;

          // TODO: After a backward task, there should be a 2 instead of a 1.

        }
        else if (robotTasks[i].val() == 2) {

          // If the current task is a backward task, then this task should not
          // be printed, but the previous task can be printed
          if (robotMovingForward[i].val() == -1) {
            temp = "BACKWARD,1;";
            printPrevious = printCurrent && true;
            printCurrent = false;
          } else {

            // If the current task is a normal forward task, then the current
            // and the previous task can be printed. The number of forward
            // steps (1 to 6) is printed, too.
            stringstream stringStreamForward;
            stringStreamForward << robotMovingForward[i].val();
            temp = "FORWARD," + stringStreamForward.str() + ";";
            printPrevious = printCurrent && true;
            printCurrent = true;
          }
        }
        // If the current task is a picking task, then this task can be printed,
        // but not the previous forward 1 task.
        else if (robotTasks[i].val() == 3) {
          temp = "PICK,1;";
          printPrevious = printCurrent && false;
          printCurrent = true;
        }
        // If the current task is a dropping task, then this task can be
        // printed, but not the previous forward 1 task.
        else if (robotTasks[i].val() == 4) {
          temp = "DROP,1;";
          printPrevious = printCurrent && false;
          printCurrent = true;
        }
        // Otherwise: Dont print only the previous one, but not the current
        // task.
        else {
          temp = "";
          printPrevious = printCurrent && true;
          printCurrent = false;
        }
        if (printPrevious) {
          os << tempPrevious;
        }
    }
    // Print at the end also the current task, if the current task should be
    // printed.
    if (printCurrent) {
      os << temp;
    }
    os << std::endl;

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




/** \brief Main-function
 *  \relates Warehouse
 */
int
main(int argc, char* argv[]) {

  /// Running the script with a dept-first-search
  Options opt("");
  opt.parse(argc,argv);
  ScriptOutput::run<Warehouse,DFS,Options>(opt);
  return 0;
}
