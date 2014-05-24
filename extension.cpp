/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "jansson/src/jansson.h"
#include <string.h>
/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

SMJansson g_SMJansson;		/**< Global singleton for extension's main interface */

JanssonObjectHandler		g_JanssonObjectHandler;
HandleType_t				htJanssonObject;

JanssonIteratorHandler      g_JanssonIteratorHandler;
HandleType_t                htJanssonIterator;

void JanssonObjectHandler::OnHandleDestroy(HandleType_t type, void *object) {
	json_decref((json_t*)object);
}

void JanssonIteratorHandler::OnHandleDestroy(HandleType_t type, void *object) {
}

bool SMJansson::SDK_OnLoad(char *error, size_t err_max, bool late)
{
	sharesys->AddNatives(myself, json_natives);
	sharesys->RegisterLibrary(myself, "jansson");

	/* Set up access rights for the 'JanssonObject' handle type */
	HandleAccess sec;
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = 0;
	sec.access[HandleAccess_Clone] = 0;
	
	htJanssonObject = g_pHandleSys->CreateType("JanssonObject", &g_JanssonObjectHandler, 0, NULL, &sec, myself->GetIdentity(), NULL);
    htJanssonIterator = g_pHandleSys->CreateType("JanssonIterator", &g_JanssonIteratorHandler, 0, NULL, &sec, myself->GetIdentity(), NULL);

	return true;
}

//native Handle:json_object();
static cell_t Native_json_object(IPluginContext *pContext, const cell_t *params) {
	json_t *object = json_object();

	Handle_t hndl = g_pHandleSys->CreateHandle(htJanssonObject, object, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndl == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Object> handle.");
	}

	return hndl;
}

//native json_object_size(Handle:hObj);
static cell_t Native_json_object_size(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	return json_object_size(object);
}

//native Handle:json_object_get(Handle:hObj, const String:sKey[]);
static cell_t Native_json_object_get(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	char *key;
	pContext->LocalToString(params[2], &key);

	// Return
	json_t *result = json_object_get(object, key);
	if(result == NULL) {
		return BAD_HANDLE;
	}

	// result is a borrowed reference, we don't know what will happen with it
	// so we increase the reference counter, which enforces the developer to
	// CloseHandle() it.
	json_incref(result);

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Object> handle.");
	}

	return hndlResult;
}

//native json_object_set(Handle:hObj, const String:sKey[], Handle:hValue);
static cell_t Native_json_object_set(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	char *key;
	pContext->LocalToString(params[2], &key);

	// Param 3
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlValue, err);
    }

	return (json_object_set_nocheck(object, key, value) == 0);
}

//native json_object_set_new(Handle:hObj, const String:sKey[], Handle:hValue);
static cell_t Native_json_object_set_new(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	char *key;
	pContext->LocalToString(params[2], &key);

	// Param 3
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlValue, err);
    }

	bool bSuccess = (json_object_set(object, key, value) == 0);
	if(bSuccess) {
		if ((err=g_pHandleSys->FreeHandle(hndlValue, NULL)) != HandleError_None)
		{
			return pContext->ThrowNativeError("Could not free <Object> handle %x (error %d)", hndlValue, err);
		}
	}

	return bSuccess;
}

//native bool:json_object_del(Handle:hObj, const String:sKey[]);
static cell_t Native_json_object_del(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	char *key;
	pContext->LocalToString(params[2], &key);

	// Return
	bool bSuccess = (json_object_del(object, key) == 0);
	return bSuccess;
}

//native bool:json_object_clear(Handle:hObj);
static cell_t Native_json_object_clear(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }


	// Return
	bool bSuccess = (json_object_clear(object) == 0);
	return bSuccess;
}

//native json_object_update(Handle:hObj, Handle:hOther);
static cell_t Native_json_object_update(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	json_t *other;
	Handle_t hndlOther = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlOther, htJanssonObject, &sec, (void **)&other)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlOther, err);
    }

	bool bSuccess = (json_object_update(object, other) == 0);
	return bSuccess;
}

//native json_object_update_existing(Handle:hObj, Handle:hOther);
static cell_t Native_json_object_update_existing(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	json_t *other;
	Handle_t hndlOther = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlOther, htJanssonObject, &sec, (void **)&other)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlOther, err);
    }

	bool bSuccess = (json_object_update_existing(object, other) == 0);
	return bSuccess;
}

//native json_object_update_missing(Handle:hObj, Handle:hOther);
static cell_t Native_json_object_update_missing(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	json_t *other;
	Handle_t hndlOther = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlOther, htJanssonObject, &sec, (void **)&other)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlOther, err);
    }

	bool bSuccess = (json_object_update_missing(object, other) == 0);
	return bSuccess;
}

//native json_object_foreach(Handle:hObj);

//native Handle:json_object_iter(Handle:hObj);
static cell_t Native_json_object_iter(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

    void *iter = json_object_iter(object);
	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonIterator, iter, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create handle for JSON Iterator.");
		return BAD_HANDLE;
	}

	return hndlResult;
}

//native Handle:json_object_iter_at(Handle:hObj, const String:key[]);
static cell_t Native_json_object_iter_at(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

    // Param 2
	char *key;
	pContext->LocalToString(params[2], &key);

    void *iter = json_object_iter_at(object, key);
	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonIterator, iter, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create handle for JSON Iterator.");
		return BAD_HANDLE;
	}

	return hndlResult;
}

//native Handle:json_object_iter_next(Handle:hObj, Handle:hIter);
static cell_t Native_json_object_iter_next(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
        return BAD_HANDLE;
    }

    // Param 2
    void *iter;
	Handle_t hndlIterator = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlIterator, htJanssonIterator, &sec, (void **)&iter)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <JSON Iterator> handle %x (error %d)", hndlIterator, err);
        return BAD_HANDLE;
    }
	
	g_pHandleSys->FreeHandle(hndlIterator, &sec);
    void *result = json_object_iter_next(object, iter);
	if(result == NULL) {
		return BAD_HANDLE;
	}	

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonIterator, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create handle for JSON Iterator.");
		return BAD_HANDLE;
	}

	return hndlResult;
}

//native Handle:json_object_iter_key(Handle:hIter, String:sKeyBuffer[], maxlength);
static cell_t Native_json_object_iter_key(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
    void *iter;
	Handle_t hndlIterator = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlIterator, htJanssonIterator, &sec, (void **)&iter)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <JSON Iterator> handle %x (error %d)", hndlIterator, err);
        return BAD_HANDLE;
    }

	// Return
	const char *result = json_object_iter_key(iter);
	if(result != NULL) {
		pContext->StringToLocalUTF8(params[2], params[3], result, NULL);
		return strlen(result);
	}

	return -1;
}

//native Handle:json_object_iter_value(Handle:hIter, String:sValueBuffer[], maxlength);
static cell_t Native_json_object_iter_value(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
    void *iter;
	Handle_t hndlIterator = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlIterator, htJanssonIterator, &sec, (void **)&iter)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <JSON Iterator> handle %x (error %d)", hndlIterator, err);
        return BAD_HANDLE;
    }

    json_t *result = json_object_iter_value(iter);

	// Return
	if(result == NULL) {
		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create handle for iterator element.");
		return BAD_HANDLE;
	}

	// result is a borrowed reference, we don't know what will happen with it
	// so we increase the reference counter, which enforces the developer to
	// CloseHandle() it.
	json_incref(result);

	return hndlResult;
}

//native bool:json_object_iter_set(Handle:hObj, Handle:hIter, Handle:hValue);
static cell_t Native_json_object_iter_set(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
        return BAD_HANDLE;
    }

    // Param 2
    void *iter;
	Handle_t hndlIterator = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlIterator, htJanssonIterator, &sec, (void **)&iter)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <JSON Iterator> handle %x (error %d)", hndlIterator, err);
        return BAD_HANDLE;
    }

 	// Param 3
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlValue, err);
        return BAD_HANDLE;
    }

    return (json_object_iter_set(object, iter, value) == 0);
}

//native bool:json_object_iter_set_new(Handle:hObj, Handle:hIter, Handle:hValue);
static cell_t Native_json_object_iter_set_new(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
        return false;
    }

    // Param 2
    void *iter;
	Handle_t hndlIterator = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlIterator, htJanssonIterator, &sec, (void **)&iter)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <JSON Iterator> handle %x (error %d)", hndlIterator, err);
        return false;
    }

 	// Param 3
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlValue, err);
        return false;
    }

	bool bSuccess = (json_object_iter_set(object, iter, value) == 0);
	if(bSuccess) {
		if ((err=g_pHandleSys->FreeHandle(hndlValue, NULL)) != HandleError_None)
		{
			pContext->ThrowNativeError("Could not free <Object> handle %x (error %d)", hndlValue, err);
			return false;
		}
	}

    return bSuccess;
}

//native Handle:json_array();
static cell_t Native_json_array(IPluginContext *pContext, const cell_t *params) {
	json_t *object = json_array();

	Handle_t hndl = g_pHandleSys->CreateHandle(htJanssonObject, object, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndl == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Array> handle.");
	}

	return hndl;
}

//native json_array_size(Handle:hArray);
static cell_t Native_json_array_size(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	return json_array_size(object);
}

//native Handle:json_array_get(Handle:hArray, iIndex);
static cell_t Native_json_array_get(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	int iIndex = params[2];
	json_t *result = json_array_get(object, iIndex);
	if(result == NULL) {
		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create handle for array element.");
		return BAD_HANDLE;
	}

	// result is a borrowed reference, we don't know what will happen with it
	// so we increase the reference counter, which enforces the developer to
	// CloseHandle() it.
	json_incref(result);

	return hndlResult;
}


//native bool:json_array_set(Handle:hArray, iIndex, Handle:hValue);
static cell_t Native_json_array_set(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: iIndex
	int iIndex = params[2];

	// Param 3: hValue
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid JSON handle %x (error %d)", hndlObject, err);
    }

	return (json_array_set(object, iIndex, value) == 0);
}

//native bool:json_array_set_new(Handle:hArray, iIndex, Handle:hValue);
static cell_t Native_json_array_set_new(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: iIndex
	int iIndex = params[2];

	// Param 3: hValue
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid JSON handle %x (error %d)", hndlObject, err);
    }

	bool bSuccess = (json_array_set(object, iIndex, value) == 0);
	if(bSuccess) {
		if ((err=g_pHandleSys->FreeHandle(hndlValue, NULL)) != HandleError_None)
		{
			pContext->ThrowNativeError("Could not free <Object> handle %x (error %d)", hndlValue, err);
			return false;
		}
	}

	return bSuccess;
}

//native json_array_append(Handle:hArray, Handle:hValue);
static cell_t Native_json_array_append(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: hValue
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid JSON handle %x (error %d)", hndlObject, err);
    }

	return (json_array_append(object, value) == 0);
}

//native json_array_append_new(Handle:hArray, Handle:hValue);
static cell_t Native_json_array_append_new(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: hValue
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid JSON handle %x (error %d)", hndlObject, err);
    }

	bool bSuccess = (json_array_append(object, value) == 0);
	if(bSuccess) {
		if ((err=g_pHandleSys->FreeHandle(hndlValue, NULL)) != HandleError_None)
		{
			pContext->ThrowNativeError("Could not free <Object> handle %x (error %d)", hndlValue, err);
			return false;
		}
	}

	return bSuccess;
}

//native json_array_insert(Handle:hArray, iIndex, Handle:hValue);
static cell_t Native_json_array_insert(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: iIndex
	int iIndex = params[2];

	// Param 3: hValue
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid JSON handle %x (error %d)", hndlObject, err);
    }

	return (json_array_insert(object, iIndex, value) == 0);
}

//native json_array_insert_new(Handle:hArray, iIndex, Handle:hValue);
static cell_t Native_json_array_insert_new(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: iIndex
	int iIndex = params[2];

	// Param 3: hValue
	json_t *value;
	Handle_t hndlValue = static_cast<Handle_t>(params[3]);
	if ((err=g_pHandleSys->ReadHandle(hndlValue, htJanssonObject, &sec, (void **)&value)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid JSON handle %x (error %d)", hndlObject, err);
    }

	bool bSuccess = (json_array_insert(object, iIndex, value) == 0);
	if(bSuccess) {
		if ((err=g_pHandleSys->FreeHandle(hndlValue, NULL)) != HandleError_None)
		{
			pContext->ThrowNativeError("Could not free <Object> handle %x (error %d)", hndlValue, err);
			return false;
		}
	}

	return bSuccess;
}

//native json_array_remove(Handle:hArray, iIndex);
static cell_t Native_json_array_remove(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: iIndex
	int iIndex = params[2];

	return (json_array_remove(object, iIndex) == 0);
}

//native json_array_clear(Handle:hArray);
static cell_t Native_json_array_clear(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	return (json_array_clear(object) == 0);
}

//native json_array_extend(Handle:hArray, Handle:hOther);
static cell_t Native_json_array_extend(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	// Param 2: hValue
	json_t *other;
	Handle_t hndlOther = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlOther, htJanssonObject, &sec, (void **)&other)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Array> handle %x (error %d)", hndlObject, err);
    }

	return (json_array_extend(object, other) == 0);
}

//native json_typeof(Handle:hObj);
static cell_t Native_json_typeof(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hArray
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
		pContext->ThrowNativeError("Invalid <JSON Object> handle %x (error %d)", hndlObject, err);
        return JSON_NULL;
    }

	if(object == NULL) {
		pContext->ThrowNativeError("JSON Object is NULL.");
		return JSON_NULL;
	}

	return json_typeof(object);
}

//native bool:json_equal(Handle:hObj, Handle:hOther);
static cell_t Native_json_equal(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hObj
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
		pContext->ThrowNativeError("Invalid <JSON Object> handle %x (error %d)", hndlObject, err);
        return false;
    }

	if(object == NULL) {
		pContext->ThrowNativeError("JSON Object is NULL.");
		return false;
	}

	// Param 2: hOther
	json_t *other;
	Handle_t hndlOther = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlOther, htJanssonObject, &sec, (void **)&other)) != HandleError_None)
    {
		pContext->ThrowNativeError("Invalid <JSON Object> handle %x (error %d)", hndlObject, err);
        return false;
    }

	if(other == NULL) {
		pContext->ThrowNativeError("JSON Object is NULL.");
		return false;
	}


	return json_equal(object, other);
}

//native Handle:json_copy(Handle:hObj);
static cell_t Native_json_copy(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hObj
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
		pContext->ThrowNativeError("Invalid <JSON Object> handle %x (error %d)", hndlObject, err);
        return BAD_HANDLE;
    }

	if(object == NULL) {
		pContext->ThrowNativeError("JSON Object is NULL.");
		return BAD_HANDLE;
	}

	json_t *copy = json_copy(object);
	if(copy == NULL) {
		pContext->ThrowNativeError("Could not copy JSON Object.");
		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, copy, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create handle for copied JSON Object.");
	}

	return hndlResult;
}

//native Handle:json_deep_copy(Handle:hObj);
static cell_t Native_json_deep_copy(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1: hObj
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
		pContext->ThrowNativeError("Invalid <JSON Object> handle %x (error %d)", hndlObject, err);
        return BAD_HANDLE;
    }

	if(object == NULL) {
		pContext->ThrowNativeError("JSON Object is NULL.");
		return BAD_HANDLE;
	}

	json_t *copy = json_deep_copy(object);
	if(copy == NULL) {
		pContext->ThrowNativeError("Could not deep copy JSON Object.");
		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, copy, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create handle for copied JSON Object.");
	}

	return hndlResult;
}

//native Handle:json_string(String:sValue[]);
static cell_t Native_json_string(IPluginContext *pContext, const cell_t *params) {
	// Param 1
	char *value;
	pContext->LocalToString(params[1], &value);

	json_t *result = json_string(value);

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <String> handle.");
	}

	return hndlResult;
}

//native json_string_value(Handle:hString, String:sValueBuffer[], maxlength);
static cell_t Native_json_string_value(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <String> handle %x (error %d)", hndlObject, err);
    }

	// Return
	const char *result = json_string_value(object);
	if(result != NULL) {
		pContext->StringToLocalUTF8(params[2], params[3], result, NULL);
		return strlen(result);
	}

	return -1;
}

//native json_string_set(Handle:hString, String:sValue[]);
static cell_t Native_json_string_set(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <String> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	char *value;
	pContext->LocalToString(params[2], &value);

	return (json_string_set_nocheck(object, value) == 0);
}

//native Handle:json_boolean(bool:bState);
static cell_t Native_json_boolean(IPluginContext *pContext, const cell_t *params) {
	bool bState = params[1];
	json_t *result = bState ? json_true() : json_false();

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Boolean> handle.");
	}

	return hndlResult;
}

//native Handle:json_true();
static cell_t Native_json_true(IPluginContext *pContext, const cell_t *params) {
	json_t *result = json_true();

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <True> handle.");
	}

	return hndlResult;
}

//native Handle:json_false();
static cell_t Native_json_false(IPluginContext *pContext, const cell_t *params) {
	json_t *result = json_false();

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <False> handle.");
	}

	return hndlResult;
}

//native Handle:json_null();
static cell_t Native_json_null(IPluginContext *pContext, const cell_t *params) {
	json_t *result = json_null();

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Null> handle.");
	}

	return hndlResult;
}

//native Handle:json_integer(value);
static cell_t Native_json_integer(IPluginContext *pContext, const cell_t *params) {
	json_t *result = json_integer(params[1]);

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Integer> handle.");
	}

	return hndlResult;
}

//native json_integer_value(Handle:hInteger);
static cell_t Native_json_integer_value(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Integer> handle %x (error %d)", hndlObject, err);
    }

	return json_integer_value(object);
}

//native json_integer_set(Handle:hInteger, value);
static cell_t Native_json_integer_set(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Integer> handle %x (error %d)", hndlObject, err);
    }

	return (json_integer_set(object, params[2]) == 0);
}

//native json_real(Float:value);
static cell_t Native_json_real(IPluginContext *pContext, const cell_t *params) {
	json_t *result = json_real(sp_ctof(params[1]));

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, result, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Real> handle.");
	}

	return hndlResult;
}

//native Float:json_real_value(Handle:hInteger);
static cell_t Native_json_real_value(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Real> handle %x (error %d)", hndlObject, err);
    }

	return sp_ftoc(json_real_value(object));
}

//native json_real_set(Handle:hFloat, Float:value);
static cell_t Native_json_real_set(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Real> handle %x (error %d)", hndlObject, err);
    }

	return (json_real_set(object, sp_ctof(params[2])) == 0);
}

//native Float:json_number_value(Handle:hNumber);
static cell_t Native_json_number_value(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Real> handle %x (error %d)", hndlObject, err);
    }

	return sp_ftoc(json_number_value(object));
}

//native Handle:json_load(const String:sJSON[]);
static cell_t Native_json_load(IPluginContext *pContext, const cell_t *params) {
	// Param 1
	char *sJSON;
	pContext->LocalToString(params[1], &sJSON);

    json_error_t error;
    json_t *object = json_loads(sJSON, 0, &error);
	if(!object) {
		g_pSM->LogError(myself, "Error in line %d, col %d: %s", error.line, error.column, error.text);
		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, object, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Object> handle.");
	}

	return hndlResult;
}

//native Handle:json_load_ex(const String:sJSON[], String:sErrorText[], maxlen, &iLine, &iColumn);
static cell_t Native_json_load_ex(IPluginContext *pContext, const cell_t *params) {
	// Param 1
	char *sJSON;
	pContext->LocalToString(params[1], &sJSON);

    json_error_t error;
    json_t *object = json_loads(sJSON, 0, &error);
	if(!object) {
		pContext->StringToLocalUTF8(params[2], params[3], error.text, NULL);

		cell_t *pLineValue;
		pContext->LocalToPhysAddr(params[4], &pLineValue);
		*pLineValue = error.line;

		cell_t *pColumnValue;		
		pContext->LocalToPhysAddr(params[5], &pColumnValue);		
		*pColumnValue = error.column;

		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, object, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Object> handle.");
	}

	return hndlResult;
}


//native Handle:json_load_file(const String:sFilePath[PLATFORM_MAX_PATH]);
static cell_t Native_json_load_file(IPluginContext *pContext, const cell_t *params) {
	// Param 1
	char *jsonfile;
	pContext->LocalToString(params[1], &jsonfile);

	char filePath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, filePath, sizeof(filePath), jsonfile);

    json_error_t error;
    json_t *object = json_load_file(filePath, 0, &error);
	if(!object) {
		g_pSM->LogError(myself, "Error in line %d, col %d: %s", error.line, error.column, error.text);
		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, object, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Object> handle.");
	}

	return hndlResult;
}

//native Handle:json_load_file_ex(const String:sFilePath[PLATFORM_MAX_PATH], String:sErrorText[], maxlen, &iLine, &iColumn);
static cell_t Native_json_load_file_ex(IPluginContext *pContext, const cell_t *params) {
	// Param 1
	char *jsonfile;
	pContext->LocalToString(params[1], &jsonfile);

	char filePath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, filePath, sizeof(filePath), jsonfile);

    json_error_t error;
    json_t *object = json_load_file(filePath, 0, &error);
	if(!object) {
		pContext->StringToLocalUTF8(params[2], params[3], error.text, NULL);

		cell_t *pLineValue;
		pContext->LocalToPhysAddr(params[4], &pLineValue);
		*pLineValue = error.line;

		cell_t *pColumnValue;		
		pContext->LocalToPhysAddr(params[5], &pColumnValue);		
		*pColumnValue = error.column;

		return BAD_HANDLE;
	}

	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, object, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(hndlResult == BAD_HANDLE) {
		pContext->ThrowNativeError("Could not create <Object> handle.");
	}

	return hndlResult;
}

//native json_dump(Handle:hObject, String:sJSON[], maxlength, iIndentWidth = 4, bool:bEnsureAscii = false, bool:bSortKeys = false, bool:bPreserveOrder = false);
static cell_t Native_json_dump(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	size_t flags = JSON_INDENT(params[4]);		// Param 4: iIndentWidth
	if(params[5] == 1) {						// Param 5: bEnsureAscii
		flags = flags | JSON_ENSURE_ASCII;
	}

	if(params[6] == 1) {						// Param 6: bSortKeys
		flags = flags | JSON_SORT_KEYS;
	}

	if(params[7] == 1) {						// Param 7: bPreserveOrder
		flags = flags | JSON_PRESERVE_ORDER;
	}

	// Return
	char *result = json_dumps(object, flags);
	if(result != NULL) {
		pContext->StringToLocalUTF8(params[2], params[3], result, NULL);
		return strlen(result);
	}

	return -1;
}

//native bool:json_dump_file(Handle:hObject, const String:sFilePath[], iIndentWidth = 4, bool:bEnsureAscii = false, bool:bSortKeys = false, bool:bPreserveOrder = false);
static cell_t Native_json_dump_file(IPluginContext *pContext, const cell_t *params) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	// Param 1
	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
    {
        return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
    }

	// Param 2
	char *jsonfile;
	pContext->LocalToString(params[2], &jsonfile);
	
	char filePath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, filePath, sizeof(filePath), jsonfile);


	size_t flags = JSON_INDENT(params[3]);		// Param 3: iIndentWidth
	if(params[4] == 1) {						// Param 4: bEnsureAscii
		flags = flags | JSON_ENSURE_ASCII;
	}

	if(params[5] == 1) {						// Param 5: bSortKeys
		flags = flags | JSON_SORT_KEYS;
	}

	if(params[6] == 1) {						// Param 6: bPreserveOrder
		flags = flags | JSON_PRESERVE_ORDER;
	}

	// Return
	bool bSuccess = (json_dump_file(object, filePath, flags) == 0);
	return bSuccess;
}


const sp_nativeinfo_t json_natives[] =
{
	// Objects
	{"json_object",								Native_json_object},
	{"json_object_size",						Native_json_object_size},
	{"json_object_get",							Native_json_object_get},
	{"json_object_set",							Native_json_object_set},
	{"json_object_set_new",						Native_json_object_set_new},
	{"json_object_del",							Native_json_object_del},
	{"json_object_clear",						Native_json_object_clear},
	{"json_object_update",						Native_json_object_update},
	{"json_object_update_existing",				Native_json_object_update_existing},
	{"json_object_update_missing",				Native_json_object_update_missing},

	// Object iteration	
	{"json_object_iter",						Native_json_object_iter},
	{"json_object_iter_at",						Native_json_object_iter_at},
	{"json_object_iter_next",					Native_json_object_iter_next},
	{"json_object_iter_key",					Native_json_object_iter_key},
	{"json_object_iter_value",					Native_json_object_iter_value},
	{"json_object_iter_set",					Native_json_object_iter_set},
	{"json_object_iter_set_new",				Native_json_object_iter_set_new},
	//{"json_object_foreach",						Native_json_object_foreach},

	// Arrays
	{"json_array",								Native_json_array},
	{"json_array_get",							Native_json_array_get},
	{"json_array_set",							Native_json_array_set},
	{"json_array_set_new",						Native_json_array_set_new},
	{"json_array_append",						Native_json_array_append},
	{"json_array_append_new",					Native_json_array_append_new},
	{"json_array_insert",						Native_json_array_insert},
	{"json_array_insert_new",					Native_json_array_insert_new},
	{"json_array_remove",						Native_json_array_remove},
	{"json_array_clear",						Native_json_array_clear},
	{"json_array_extend",						Native_json_array_extend},
	{"json_array_size",							Native_json_array_size},

	// Type
	{"json_typeof",								Native_json_typeof},

	// Equality
	{"json_equal",								Native_json_equal},

	// Copying
	{"json_copy",								Native_json_copy},
	{"json_deep_copy",							Native_json_deep_copy},

	// Values
	{"json_boolean",							Native_json_boolean},
	{"json_true",								Native_json_true},
	{"json_false",								Native_json_false},
	{"json_null",								Native_json_null},	

	{"json_string",								Native_json_string},
	{"json_string_value",						Native_json_string_value},
	{"json_string_set",							Native_json_string_set},

	{"json_integer",							Native_json_integer},
	{"json_integer_value",						Native_json_integer_value},
	{"json_integer_set",						Native_json_integer_set},

	{"json_real",								Native_json_real},
	{"json_real_value",							Native_json_real_value},
	{"json_real_set",							Native_json_real_set},
	{"json_number_value",						Native_json_number_value},

	// Encoding
	{"json_dump",								Native_json_dump},
	{"json_dump_file",							Native_json_dump_file},

	// Decoding
	{"json_load",								Native_json_load},
	{"json_load_ex",							Native_json_load_ex},
	{"json_load_file",							Native_json_load_file},
	{"json_load_file_ex",						Native_json_load_file_ex},

	// Building objects & arrays
	//{"json_unpack",							Native_json_unpack},

	{NULL,				NULL}
};

SMEXT_LINK(&g_SMJansson);


















