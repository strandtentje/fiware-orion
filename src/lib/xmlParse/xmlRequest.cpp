/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string.h>                 // strstr
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "rest/restReply.h"
#include "rest/OrionError.h"
#include "xmlParse/xmlRegisterContextRequest.h"
#include "xmlParse/xmlRegisterContextResponse.h"
#include "xmlParse/xmlDiscoverContextAvailabilityRequest.h"
#include "xmlParse/xmlSubscribeContextAvailabilityRequest.h"
#include "xmlParse/xmlUnsubscribeContextAvailabilityRequest.h"
#include "xmlParse/xmlNotifyContextRequest.h"
#include "xmlParse/xmlNotifyContextAvailabilityRequest.h"
#include "xmlParse/xmlUpdateContextAvailabilitySubscriptionRequest.h"
#include "xmlParse/xmlQueryContextRequest.h"
#include "xmlParse/xmlQueryContextResponse.h"
#include "xmlParse/xmlSubscribeContextRequest.h"
#include "xmlParse/xmlUnsubscribeContextRequest.h"
#include "xmlParse/xmlUpdateContextSubscriptionRequest.h"
#include "xmlParse/xmlUpdateContextRequest.h"
#include "xmlParse/xmlUpdateContextResponse.h"
#include "xmlParse/xmlRegisterProviderRequest.h"
#include "xmlParse/xmlUpdateContextElementRequest.h"
#include "xmlParse/xmlAppendContextElementRequest.h"
#include "xmlParse/xmlUpdateContextAttributeRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlRequest.h"



/* ****************************************************************************
*
* xmlRequest - the vector used to parse the payload in XML
*
*  RequestType     type        - the type of request (6 ngsi9, 6 ngsi10)
*  std::string     method      - the method/verb of the request (POST, PUT, etc)
*  std::string     keyword     - first word in payload
*  XmlNode*        parseVector - important vector for the parsing of each element
*  RequestInit     init        - init function of the parsing
*  RequestRelease  release     - cleanup function called after the parsing is completed
*  RequestPresent  present     - debugging tool
*  RequestCheck    check       - function to verify the the payload is ngsi compliant
*
*/
#define FUNCS(prefix) prefix##ParseVector, prefix##Init,    \
                      prefix##Release,     prefix##Present, \
                      prefix##Check

static XmlRequest xmlRequest[] =
{
  // NGSI9
  { RegisterContext,                       "POST", "registerContextRequest",                       FUNCS(rcr)   },
  { RegisterContext,                       "*",    "registerContextRequest",                       FUNCS(rcr)   },
  { DiscoverContextAvailability,           "POST", "discoverContextAvailabilityRequest",           FUNCS(dcar)  },
  { DiscoverContextAvailability,           "*",    "discoverContextAvailabilityRequest",           FUNCS(dcar)  },
  { SubscribeContextAvailability,          "POST", "subscribeContextAvailabilityRequest",          FUNCS(scar)  },
  { SubscribeContextAvailability,          "*",    "subscribeContextAvailabilityRequest",          FUNCS(scar)  },
  { UnsubscribeContextAvailability,        "POST", "unsubscribeContextAvailabilityRequest",        FUNCS(ucar)  },
  { UnsubscribeContextAvailability,        "*",    "unsubscribeContextAvailabilityRequest",        FUNCS(ucar)  },
  { UpdateContextAvailabilitySubscription, "POST", "updateContextAvailabilitySubscriptionRequest", FUNCS(ucas)  },
  { UpdateContextAvailabilitySubscription, "*",    "updateContextAvailabilitySubscriptionRequest", FUNCS(ucas)  },
  { NotifyContextAvailability,             "POST", "notifyContextAvailabilityRequest",             FUNCS(ncar)  },
  { NotifyContextAvailability,             "*",    "notifyContextAvailabilityRequest",             FUNCS(ncar)  },

  // NGSI10
  { UpdateContext,                         "POST", "updateContextRequest",                         FUNCS(upcr)  },
  { UpdateContext,                         "*",    "updateContextRequest",                         FUNCS(upcr)  },
  { QueryContext,                          "POST", "queryContextRequest",                          FUNCS(qcr)   },
  { QueryContext,                          "*",    "queryContextRequest",                          FUNCS(qcr)   },
  { SubscribeContext,                      "POST", "subscribeContextRequest",                      FUNCS(scr)   },
  { SubscribeContext,                      "*",    "subscribeContextRequest",                      FUNCS(scr)   },
  { UpdateContextSubscription,             "POST", "updateContextSubscriptionRequest",             FUNCS(ucsr)  },
  { UpdateContextSubscription,             "*",    "updateContextSubscriptionRequest",             FUNCS(ucsr)  },
  { UnsubscribeContext,                    "POST", "unsubscribeContextRequest",                    FUNCS(uncr)  },
  { UnsubscribeContext,                    "*",    "unsubscribeContextRequest",                    FUNCS(uncr)  },
  { NotifyContext,                         "POST", "notifyContextRequest",                         FUNCS(ncr)   },
  { NotifyContext,                         "*",    "notifyContextRequest",                         FUNCS(ncr)   },

  // Convenience
  { ContextEntitiesByEntityId,             "POST", "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntitiesByEntityId,             "*",    "registerProviderRequest",                      FUNCS(rpr)   },
  { EntityByIdAttributeByName,             "POST", "registerProviderRequest",                      FUNCS(rpr)   },
  { EntityByIdAttributeByName,             "*",    "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityAttributes,               "POST", "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityAttributes,               "*",    "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityTypes,                    "POST", "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityTypes,                    "*",    "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityTypeAttributeContainer,   "POST", "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityTypeAttributeContainer,   "*",    "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityTypeAttribute,            "POST", "registerProviderRequest",                      FUNCS(rpr)   },
  { ContextEntityTypeAttribute,            "*",    "registerProviderRequest",                      FUNCS(rpr)   },

  { UpdateContextElement,                  "POST", "updateContextElementRequest",                  FUNCS(ucer)  },
  { IndividualContextEntity,               "PUT",  "updateContextElementRequest",                  FUNCS(ucer)  },
  { IndividualContextEntity,               "POST", "appendContextElementRequest",                  FUNCS(acer)  },
  { AllContextEntities,                    "POST", "appendContextElementRequest",                  FUNCS(acer)  },

  { IndividualContextEntityAttribute,      "POST", "updateContextAttributeRequest",                FUNCS(upcar) },
  { IndividualContextEntityAttribute,      "PUT",  "updateContextAttributeRequest",                FUNCS(upcar) },
  { AttributeValueInstance,                "PUT",  "updateContextAttributeRequest",                FUNCS(upcar) },
  { IndividualContextEntityAttributes,     "POST", "appendContextElementRequest",                  FUNCS(acer)  },
  { IndividualContextEntityAttributes,     "PUT",  "updateContextElementRequest",                  FUNCS(ucer)  },
  { AppendContextElement,                  "POST", "appendContextElementRequest",                  FUNCS(acer)  },
  { UpdateContextAttribute,                "POST", "updateContextAttributeRequest",                FUNCS(upcar) },

  { SubscribeContext,                      "POST", "subscribeContextRequest",                      FUNCS(scr)   },
  { Ngsi10SubscriptionsConvOp,             "PUT",  "updateContextSubscriptionRequest",             FUNCS(ucsr)  },
  { Ngsi9SubscriptionsConvOp,              "POST", "subscribeContextAvailabilityRequest",          FUNCS(scar)  },
  { Ngsi9SubscriptionsConvOp,              "PUT",  "updateContextvailabilitySubscriptionRequest",  FUNCS(ucas)  },

  { AllEntitiesWithTypeAndId,              "PUT",  "updateContextElementRequest",                  FUNCS(ucer)  },
  { AllEntitiesWithTypeAndId,              "POST", "appendContextElementRequest",                  FUNCS(acer)  },
  { AllEntitiesWithTypeAndId,              "*",    "",                                             FUNCS(ucer)  },

  { IndividualContextEntityAttributeWithTypeAndId, "POST", "updateContextAttributeRequest",        FUNCS(upcar) },
  { IndividualContextEntityAttributeWithTypeAndId, "PUT",  "updateContextAttributeRequest",        FUNCS(upcar) },

  { AttributeValueInstanceWithTypeAndId,           "PUT",  "updateContextAttributeRequest",        FUNCS(upcar) },
  { AttributeValueInstanceWithTypeAndId,           "POST", "updateContextAttributeRequest",        FUNCS(upcar) },

  { ContextEntitiesByEntityIdAndType,              "POST", "registerProviderRequest",              FUNCS(rpr)   },
  { EntityByIdAttributeByNameIdAndType,            "POST", "registerProviderRequest",              FUNCS(rpr)   },

  // Responses
  { RegisterResponse,                      "POST", "registerContextResponse",                      FUNCS(rcrs)  },
  { RtQueryContextResponse,                "POST", "queryContextResponse",                         FUNCS(qcrs)  },
  { RtUpdateContextResponse,               "POST", "updateContextResponse",                        FUNCS(upcrs) },
  { RtUpdateContextResponse,               "PUT",  "updateContextResponse",                        FUNCS(upcrs) },

  // Without payload
  { LogRequest,                            "*", "", NULL, NULL, NULL, NULL, NULL },
  { VersionRequest,                        "*", "", NULL, NULL, NULL, NULL, NULL },
  { ExitRequest,                           "*", "", NULL, NULL, NULL, NULL, NULL },
  { InvalidRequest,                        "*", "", NULL, NULL, NULL, NULL, NULL }
};



/* ****************************************************************************
*
* xmlRequestGet -
*/
static XmlRequest* xmlRequestGet(RequestType request, std::string method)
{
  for (unsigned int ix = 0; ix < sizeof(xmlRequest) / sizeof(xmlRequest[0]); ++ix)
  {
    if ((request == xmlRequest[ix].type) && ((xmlRequest[ix].method == method) || (xmlRequest[ix].method == "*")))
    {
      if (xmlRequest[ix].parseVector != NULL)
        LM_T(LmtHttpRequest, ("Found xmlRequest of type %d, method '%s' - index %d (%s)",
                              request,
                              method.c_str(),
                              ix,
                              xmlRequest[ix].parseVector[0].path.c_str()));
      return &xmlRequest[ix];
    }
  }

  std::string details = std::string("no request found for RequestType '") + requestType(request) + "', method '" + method + "'";
  alarmMgr.badInput(clientIp, details);

  return NULL;
}



/* ****************************************************************************
*
* xmlTreat -
*/
std::string xmlTreat
(
  const char*      content,
  ConnectionInfo*  ciP,
  ParseData*       parseDataP,
  RequestType      request,
  std::string      payloadWord,
  XmlRequest**     reqPP,
  std::string*     errorMsgP
)
{
  xml_document<>   doc;
  char*            xmlPayload = (char*) content;
  struct timespec  start;
  struct timespec  end;


  //
  // If the payload is empty, the XML parsing library does an assert
  // and the broker dies.
  // Therefore, this check here is important, to avoid death.
  // 
  // 'OK' is returned as there is no error to send a request without payload.
  //
  if ((content == NULL) || (*content == 0))
  {
    return "OK";
  }

  try
  {
    if (timingStatistics)
    {
      clock_gettime(CLOCK_REALTIME, &start);
    }

    doc.parse<0>(xmlPayload);
  }
  catch (parse_error& e)
  {
    std::string details    = std::string("'") + content + "', '" + e.what() + "'";
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", "unknown", SccBadRequest, "XML Parse Error");

    alarmMgr.badInput(clientIp, details);

    if (errorMsgP)
    {
      *errorMsgP = std::string("XML parse error exception: ") + e.what();
    }

    return errorReply;
  }
  // in this case the try/catch block is not using a 'catch (const std::exception &e)' clause, as we are not using
  // e.what(), so it wouldn't be useful
  catch (...)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", "unknown", SccBadRequest, "XML Parse Error");
    alarmMgr.badInput(clientIp, content);

    if (errorMsgP)
    {
      *errorMsgP = std::string("XML parse generic exception");
    }

    return errorReply;
  }

  xml_node<>*   father = doc.first_node();
  XmlRequest*   reqP   = xmlRequestGet(request, ciP->method);

  ciP->parseDataP = parseDataP;

  if (father == NULL)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", "unknown", SccBadRequest, "XML Parse Error");
    alarmMgr.badInput(clientIp, "XML parse error");

    if (errorMsgP)
    {
      *errorMsgP = std::string("XML parse error: invalid XML input");
    }

    return errorReply;
  }

  if (reqP == NULL)
  {
    std::string errorReply =
      restErrorReplyGet(
        ciP,
        ciP->outFormat,
        "",
        requestType(request),
        SccBadRequest,
        std::string("Sorry, no request treating object found for RequestType /") +
        requestType(request) + "/, method /" + ciP->method + "/");

    char rtV[STRING_SIZE_FOR_INT];
    snprintf(rtV, sizeof(rtV), "%d", request);
    std::string details = std::string("no request treating object found for RequestType ") + rtV + " (" + requestType(request) + "), method " + ciP->method;
    alarmMgr.badInput(clientIp, details);

    if (errorMsgP)
    {
      *errorMsgP = std::string("Unable to treat ") + requestType(request) + " requests";
    }

    return errorReply;
  }


  if (reqPP != NULL)
  {
    *reqPP = reqP;
  }

  //
  // Checking that the payload matches the URL
  //
  if (((ciP->verb == POST) || (ciP->verb == PUT)) && (payloadWord.length() != 0))
  {
    std::string  errorReply;
    char*        payloadStart = (char*) content;

    // Skip '<?xml version="1.0" encoding="UTF-8"?> ' ?
    if (strncmp(payloadStart, "<?xml", 5) == 0)
    {
      ++payloadStart;
      payloadStart = strstr(payloadStart, "<");
    }

    // Skip '<'
    if (*payloadStart == '<')
    {
       ++payloadStart;
    }

    if (strncasecmp(payloadWord.c_str(), payloadStart, payloadWord.length()) != 0)
    {
      errorReply  = restErrorReplyGet(ciP,
                                      ciP->outFormat,
                                      "",
                                      reqP->keyword,
                                      SccBadRequest,
                                      std::string("Expected /") + payloadWord +
                                        "/ payload, got /" + payloadStart + "/");

      std::string details = std::string("invalid payload: wanted: ") + "'" + payloadWord + "', got '" + payloadStart + "'";
      alarmMgr.badInput(clientIp, details);

      if (errorMsgP)
      {
        *errorMsgP = std::string("Bad Input (invalid payload, expecting '") +
          payloadWord + "', got '" + payloadStart + "')";
      }

      return errorReply;
    }
  }

  if (reqP->init == NULL)  // No payload treating function
  {
    return "OK";
  }

  reqP->init(parseDataP);
  ciP->httpStatusCode = SccOk;

  xmlParse(ciP, NULL, father, "", "", reqP->parseVector, parseDataP, errorMsgP);

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &end);
    clock_difftime(&end, &start, &threadLastTimeStat.xmlParseTime);
  }

  if (ciP->httpStatusCode != SccOk)
  {
    alarmMgr.badInput(clientIp, "XML parse error");

    return restErrorReplyGet(ciP, ciP->outFormat, "", payloadWord, ciP->httpStatusCode, ciP->answer);
  }

  LM_T(LmtParseCheck, ("Calling check for XML parsed tree (%s)", ciP->payloadWord));
  std::string check = reqP->check(parseDataP, ciP);

  if (check != "OK")
  {
    std::string details = reqP->keyword + ": " + check;

    alarmMgr.badInput(clientIp, details);

    if (errorMsgP)
    {
      *errorMsgP = std::string("Bad Input: ") + check;
    }
  }

  return check;
}
