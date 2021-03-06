#include <list>
#include "tPassToPoint.hpp"
#include "skills/skillSet.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

#include <fstream>
#include <stdio.h>
#include <ssl_common/geometry.hpp>
#include <skills/skillSet.h>
#include "ros/ros.h"


#define THRES (0.8f)

namespace Strategy
{
  TPassToPoint::TPassToPoint(int botID) : Tactic( botID) { 
   iState=GOTOBALL;
  }

  TPassToPoint::~TPassToPoint() { } 

  bool TPassToPoint::isCompleted(const BeliefState &bs,const Tactic::Param& tParam) const {
    if(iState == FINISHED) 
    {
      fstream f;
      f.open("/home/gunjan/catkin_ws/src/play/iState.txt",fstream::out| fstream::app);
      f<<"finished pass \n";
      f.close();
      return true;
    }
    else 
      {
       fstream f;
        f.open("/home/gunjan/catkin_ws/src/play/iState.txt",fstream::out| fstream::app);
        f<<"not finished pass\n";
        f.close();
        return false;
      }
  }
  
  inline bool TPassToPoint::isActiveTactic(void) const {
    return true;
  }

  int TPassToPoint::chooseBestBot(const BeliefState &state, std::list<int>& freeBots, const Param& tParam, int prevID) const {
  
    int minv = *(freeBots.begin());
    float mindis = -1;
    for (std::list<int>::const_iterator it = freeBots.begin(); it != freeBots.end(); ++it)
    {
      Vector2D<int> homePos(state.homePos[*it].x, state.homePos[*it].y);
      Vector2D<int> ballPos(state.ballPos.x, state.ballPos.y);

      float dis_from_ball = (homePos - ballPos).absSq();
      if (mindis < 0) {
        mindis = dis_from_ball;
        minv = *it; 
      }
      else if(dis_from_ball < mindis) {
        mindis = dis_from_ball;
        minv = *it;
      }
    }
    assert(mindis >= 0.0f);
    return minv;
  }

  gr_Robot_Command TPassToPoint::execute(const BeliefState &state, const Tactic::Param& tParam) {
    
    Vector2D<int> point(tParam.PassToPointP.x, tParam.PassToPointP.y);
    Vector2D<int> ballPos(state.ballPos.x, state.ballPos.y);
    Vector2D<int> botPos(state.homePos[botID].x, state.homePos[botID].y);
    
    float dist = Vector2D<int>::dist(ballPos, botPos);

    float angleToTurn = normalizeAngle(state.homePos[botID].theta - Vector2D<int>::angle(point, ballPos));       

    float pointDis = Vector2D<int>::dist(botPos, point);
    float goalBotAngle = Vector2D<int>::angle(point, botPos);
    float ballBotAngle = Vector2D<int>::angle(ballPos, botPos);
    float angle = Vector2D<int>::angle(point, botPos);

    float angleUp = angle + asin(BOT_RADIUS / (10.0 * pointDis));
    float angleDown = angle - asin(BOT_RADIUS / (10.0 * pointDis));

    Strategy::SkillSet::SkillID sID;
    SkillSet::SParam sParam;
    if(iState==FINISHED)
    {
      iState=FINISHED;
    }
    else
    {
       if (dist >= DRIBBLER_BALL_THRESH) {
          iState=GOTOBALL;     
        }
        
        else if (state.homePos[botID].theta > angleUp || state.homePos[botID].theta < angleDown){
          iState=TURNING;
        }
        else {
          iState=PASSING;
        }  
    }
      
    switch(iState)
    {
      case GOTOBALL:
      {
        sID = SkillSet::GoToBall;
        return SkillSet::instance()->executeSkill(sID, sParam, state, botID);
        break;
      }
      case TURNING:
      {
        sID = SkillSet::TurnToPoint;
        sParam.TurnToPointP.x = point.x;
        sParam.TurnToPointP.y = point.y;
        sParam.TurnToPointP.max_omega = MAX_BOT_OMEGA;
        return SkillSet::instance()->executeSkill(sID, sParam, state, botID);
        break;
      }  
      case PASSING:
      {

        float power = pointDis / (HALF_FIELD_MAXX / 8.0);
        power = power > 6.0 ? 6.0 : power;
        power = power < 3.0 ? 3.0 : power;
        sID = SkillSet::Kick;
        sParam.KickP.power = power; 
        iState = FINISHED;
        return SkillSet::instance()->executeSkill(sID, sParam, state, botID);
        break;
      }        
      default:
      {
        iState=FINISHED;
      }
    }
  }  

  Tactic::Param TPassToPoint::paramFromJSON(string json) {
    using namespace rapidjson;
      Tactic::Param tParam;
      Document d;
      d.Parse(json.c_str());
      tParam.PassToPointP.x = d["x"].GetDouble();
      tParam.PassToPointP.y = d["y"].GetDouble();

      return tParam;
 }

  string TPassToPoint::paramToJSON(Tactic::Param tParam) {
    using namespace rapidjson;
      StringBuffer buffer;
      Writer<StringBuffer> w(buffer);
      w.StartObject();
      w.String("x");
      w.Double(tParam.PassToPointP.x);
      w.String("y");
      w.Double(tParam.PassToPointP.y);
      w.EndObject();
      return buffer.GetString();
  }    

} 
