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
#include <jansson.h>
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

typedef int (*ObjectUpdateFunc)(json_t *object, json_t *other);

// Shared by the update natives that come after Jansson 2.6: reads hObj and
// hOther, applies update, and closes hOther on success if bCloseOther is set.
static cell_t UpdateObject(IPluginContext *pContext, const cell_t *params, ObjectUpdateFunc update, bool bCloseOther) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	json_t *object;
	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)&object)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
	}

	json_t *other;
	Handle_t hndlOther = static_cast<Handle_t>(params[2]);
	if ((err=g_pHandleSys->ReadHandle(hndlOther, htJanssonObject, &sec, (void **)&other)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlOther, err);
	}

	if (update(object, other) != 0) {
		return false;
	}

	if (bCloseOther && (err=g_pHandleSys->FreeHandle(hndlOther, NULL)) != HandleError_None) {
		return pContext->ThrowNativeError("Could not free <Object> handle %x (error %d)", hndlOther, err);
	}

	return true;
}

//native bool json_object_update_new(Handle hObj, Handle hOther);
static cell_t Native_json_object_update_new(IPluginContext *pContext, const cell_t *params) {
	return UpdateObject(pContext, params, json_object_update, true);
}

//native bool json_object_update_existing_new(Handle hObj, Handle hOther);
static cell_t Native_json_object_update_existing_new(IPluginContext *pContext, const cell_t *params) {
	return UpdateObject(pContext, params, json_object_update_existing, true);
}

//native bool json_object_update_missing_new(Handle hObj, Handle hOther);
static cell_t Native_json_object_update_missing_new(IPluginContext *pContext, const cell_t *params) {
	return UpdateObject(pContext, params, json_object_update_missing, true);
}

//native bool json_object_update_recursive(Handle hObj, Handle hOther);
static cell_t Native_json_object_update_recursive(IPluginContext *pContext, const cell_t *params) {
	return UpdateObject(pContext, params, json_object_update_recursive, false);
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

// The flags SMJansson passes on to Jansson. JSON_ALLOW_NUL and JSON_EMBED are
// left out: SourcePawn strings end at the first NUL and dumps go to a buffer.
static const size_t kLoadFlags = JSON_REJECT_DUPLICATES | JSON_DISABLE_EOF_CHECK | JSON_DECODE_ANY | JSON_DECODE_INT_AS_REAL;
static const size_t kDumpFlags = JSON_MAX_INDENT | JSON_COMPACT | JSON_ENSURE_ASCII | JSON_SORT_KEYS | JSON_ENCODE_ANY | JSON_ESCAPE_SLASH | JSON_REAL_PRECISION(31);

// Reads the flags in parameter iParam, or 0 if the caller passed fewer
// parameters (a plugin compiled against an older smjansson.inc).
// Throws and returns false for flags SMJansson does not support.
static bool ReadFlags(IPluginContext *pContext, const cell_t *params, int iParam, size_t supported, size_t *flags) {
	*flags = 0;
	if (params[0] < iParam) {
		return true;
	}

	size_t requested = static_cast<size_t>(static_cast<uint32_t>(params[iParam]));
	if ((requested & ~supported) != 0) {
		pContext->ThrowNativeError("Unsupported flags %x", static_cast<unsigned int>(requested & ~supported));
		return false;
	}

	*flags = requested;
	return true;
}

static cell_t CreateObjectHandle(IPluginContext *pContext, json_t *object) {
	Handle_t hndlResult = g_pHandleSys->CreateHandle(htJanssonObject, object, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if(hndlResult == BAD_HANDLE) {
		json_decref(object);
		pContext->ThrowNativeError("Could not create <Object> handle.");
	}

	return hndlResult;
}

// Fills the error parameters of json_load_ex and json_load_file_ex:
// sErrorText (2, 3), iLine (4), iColumn (5) and, if passed, iErrorCode (7).
static void WriteLoadError(IPluginContext *pContext, const cell_t *params, const json_error_t &error) {
	pContext->StringToLocalUTF8(params[2], params[3], error.text, NULL);

	cell_t *pLineValue;
	pContext->LocalToPhysAddr(params[4], &pLineValue);
	*pLineValue = error.line;

	cell_t *pColumnValue;
	pContext->LocalToPhysAddr(params[5], &pColumnValue);
	*pColumnValue = error.column;

	if (params[0] >= 7) {
		cell_t *pErrorCode;
		pContext->LocalToPhysAddr(params[7], &pErrorCode);
		*pErrorCode = json_error_code(&error);
	}
}

static json_t *LoadFile(IPluginContext *pContext, cell_t pathParam, size_t flags, json_error_t *error) {
	char *jsonfile;
	pContext->LocalToString(pathParam, &jsonfile);

	char filePath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, filePath, sizeof(filePath), jsonfile);

	return json_load_file(filePath, flags, error);
}

//native Handle json_load(const char[] sJSON, int iFlags = 0);
static cell_t Native_json_load(IPluginContext *pContext, const cell_t *params) {
	size_t flags;
	if (!ReadFlags(pContext, params, 2, kLoadFlags, &flags)) {
		return BAD_HANDLE;
	}

	char *sJSON;
	pContext->LocalToString(params[1], &sJSON);

	json_error_t error;
	json_t *object = json_loads(sJSON, flags, &error);
	if(!object) {
		g_pSM->LogError(myself, "Error in line %d, col %d: %s", error.line, error.column, error.text);
		return BAD_HANDLE;
	}

	return CreateObjectHandle(pContext, object);
}

//native Handle json_load_ex(const char[] sJSON, char[] sErrorText, int maxlen, int &iLine, int &iColumn, int iFlags = 0, json_error_code &iErrorCode = json_error_unknown);
static cell_t Native_json_load_ex(IPluginContext *pContext, const cell_t *params) {
	size_t flags;
	if (!ReadFlags(pContext, params, 6, kLoadFlags, &flags)) {
		return BAD_HANDLE;
	}

	char *sJSON;
	pContext->LocalToString(params[1], &sJSON);

	json_error_t error;
	json_t *object = json_loads(sJSON, flags, &error);
	if(!object) {
		WriteLoadError(pContext, params, error);
		return BAD_HANDLE;
	}

	return CreateObjectHandle(pContext, object);
}

//native Handle json_load_file(const char sFilePath[PLATFORM_MAX_PATH], int iFlags = 0);
static cell_t Native_json_load_file(IPluginContext *pContext, const cell_t *params) {
	size_t flags;
	if (!ReadFlags(pContext, params, 2, kLoadFlags, &flags)) {
		return BAD_HANDLE;
	}

	json_error_t error;
	json_t *object = LoadFile(pContext, params[1], flags, &error);
	if(!object) {
		g_pSM->LogError(myself, "Error in line %d, col %d: %s", error.line, error.column, error.text);
		return BAD_HANDLE;
	}

	return CreateObjectHandle(pContext, object);
}

//native Handle json_load_file_ex(const char sFilePath[PLATFORM_MAX_PATH], char[] sErrorText, int maxlen, int &iLine, int &iColumn, int iFlags = 0, json_error_code &iErrorCode = json_error_unknown);
static cell_t Native_json_load_file_ex(IPluginContext *pContext, const cell_t *params) {
	size_t flags;
	if (!ReadFlags(pContext, params, 6, kLoadFlags, &flags)) {
		return BAD_HANDLE;
	}

	json_error_t error;
	json_t *object = LoadFile(pContext, params[1], flags, &error);
	if(!object) {
		WriteLoadError(pContext, params, error);
		return BAD_HANDLE;
	}

	return CreateObjectHandle(pContext, object);
}

// Builds the Jansson flags of json_dump and json_dump_file from their
// parameters iIndentWidth, bEnsureAscii, bSortKeys and bPreserveOrder,
// starting at parameter iFirst.
static size_t LegacyDumpFlags(const cell_t *params, int iFirst) {
	size_t flags = JSON_INDENT(params[iFirst]);
	if(params[iFirst + 1] == 1) {
		flags = flags | JSON_ENSURE_ASCII;
	}

	if(params[iFirst + 2] == 1) {
		flags = flags | JSON_SORT_KEYS;
	}

	if(params[iFirst + 3] == 1) {
		flags = flags | JSON_PRESERVE_ORDER;
	}

	return flags;
}

static bool ReadDumpObject(IPluginContext *pContext, const cell_t *params, json_t **object) {
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Handle_t hndlObject = static_cast<Handle_t>(params[1]);
	if ((err=g_pHandleSys->ReadHandle(hndlObject, htJanssonObject, &sec, (void **)object)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid <Object> handle %x (error %d)", hndlObject, err);
		return false;
	}

	return true;
}

// Writes the JSON text of object into the buffer in parameters 2 and 3.
// Returns the length of the whole text, or -1 if Jansson could not encode it.
static cell_t DumpToBuffer(IPluginContext *pContext, const cell_t *params, json_t *object, size_t flags) {
	char *result = json_dumps(object, flags);
	if(result == NULL) {
		return -1;
	}

	pContext->StringToLocalUTF8(params[2], params[3], result, NULL);
	size_t length = strlen(result);
	free(result);
	return length;
}

static bool DumpToFile(IPluginContext *pContext, cell_t pathParam, json_t *object, size_t flags) {
	char *jsonfile;
	pContext->LocalToString(pathParam, &jsonfile);

	char filePath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, filePath, sizeof(filePath), jsonfile);

	return json_dump_file(object, filePath, flags) == 0;
}

//native int json_dump(Handle hObject, char[] sJSON, int maxlength, int iIndentWidth = 4, bool bEnsureAscii = false, bool bSortKeys = false, bool bPreserveOrder = false);
static cell_t Native_json_dump(IPluginContext *pContext, const cell_t *params) {
	json_t *object;
	if (!ReadDumpObject(pContext, params, &object)) {
		return -1;
	}

	return DumpToBuffer(pContext, params, object, LegacyDumpFlags(params, 4));
}

//native int json_dump_ex(Handle hObject, char[] sJSON, int maxlength, int iFlags);
static cell_t Native_json_dump_ex(IPluginContext *pContext, const cell_t *params) {
	size_t flags;
	if (!ReadFlags(pContext, params, 4, kDumpFlags, &flags)) {
		return -1;
	}

	json_t *object;
	if (!ReadDumpObject(pContext, params, &object)) {
		return -1;
	}

	return DumpToBuffer(pContext, params, object, flags);
}

//native bool json_dump_file(Handle hObject, const char[] sFilePath, int iIndentWidth = 4, bool bEnsureAscii = false, bool bSortKeys = false, bool bPreserveOrder = false);
static cell_t Native_json_dump_file(IPluginContext *pContext, const cell_t *params) {
	json_t *object;
	if (!ReadDumpObject(pContext, params, &object)) {
		return false;
	}

	return DumpToFile(pContext, params[2], object, LegacyDumpFlags(params, 3));
}

//native bool json_dump_file_ex(Handle hObject, const char[] sFilePath, int iFlags);
static cell_t Native_json_dump_file_ex(IPluginContext *pContext, const cell_t *params) {
	size_t flags;
	if (!ReadFlags(pContext, params, 3, kDumpFlags, &flags)) {
		return false;
	}

	json_t *object;
	if (!ReadDumpObject(pContext, params, &object)) {
		return false;
	}

	return DumpToFile(pContext, params[2], object, flags);
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
	{"json_object_update_new",					Native_json_object_update_new},
	{"json_object_update_existing_new",			Native_json_object_update_existing_new},
	{"json_object_update_missing_new",			Native_json_object_update_missing_new},
	{"json_object_update_recursive",			Native_json_object_update_recursive},

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
	{"json_dump_ex",							Native_json_dump_ex},
	{"json_dump_file_ex",						Native_json_dump_file_ex},

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


















