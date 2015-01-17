#include <Sound.h>
#include <InterfaceDefs.h>
extern "C" _EXPORT status_t Identify(BPositionIO *inSource,const translation_format *inFormat,
  BMessage *ioExtension,translator_info *outInfo, uint32 outType);
  
extern "C" _EXPORT status_t Translate(BPositionIO *inSource,const translator_info *inInfo,
  BMessage *ioExtension,uint32 outType,BPositionIO *outDestination);

extern "C" _EXPORT char translatorInfo[] = "Plain Text/StyledEdit Translator, part of Translator Plus (http://transplus.sourceforge.net)";
extern "C" _EXPORT char translatorName[] = "Plain Text/StyledEdit Translator";
extern "C" _EXPORT int32 translatorVersion = 100;

#include "TransWP.h"

#define CRASH (new BAlert("ck","oops","OK"))->Go();

void pullSEInfo(BNode *node,TranslatorWP *WP,size_t length);

status_t Identify(BPositionIO *inSource,const translation_format *inFormat,
  BMessage *ioExtension,translator_info *outInfo, uint32 outType) {
	if (outType == 0)
		outType = 'TRWP';
	if (!((outType == 'TEXT') || (outType == 'CSTR') || (outType == 'TRWP'))) {
		return B_NO_TRANSLATOR;
	}
	if (inFormat == NULL)
		goto is_plain_text;
	if ((outType == 'TRWP') || (outType == 0) || (strncmp("text",inFormat->MIME,4) == 0) || ((inFormat->type == 'TEXT') || (inFormat->type == 'CSTR') || (inFormat->type == B_TRANSLATOR_TEXT))) {
		  is_plain_text:
			if (outInfo == NULL)
				return B_OK;
			outInfo->type = 'TEXT';
			outInfo->group = B_TRANSLATOR_TEXT;
			outInfo->quality = 0.5;
			outInfo->capability = 1.0;
			strcpy(outInfo->name,"Plain text");
			strcpy(outInfo->MIME,"text/plain");
			return B_OK;
	} else {
		BMessage msg;
		if (msg.Unflatten(inSource) != B_OK)
			return B_NO_TRANSLATOR;
		if (validate_instantiation(&msg,"TranslatorWP") == false)
			return B_NO_TRANSLATOR;
		outInfo->type = 'TRWP';
		outInfo->group = B_TRANSLATOR_TEXT;
		outInfo->quality = 1.0;
		outInfo->capability = 1.0;
		strcpy(outInfo->name,"TranslatorWP");
		strcpy(outInfo->MIME,"text/trans-WP");
		return B_OK;
	}
	return B_NO_TRANSLATOR;
}

status_t Translate(BPositionIO *inSource,const translator_info *inInfo,
  BMessage *ioExtension,uint32 outType,BPositionIO *outDestination) {
	if (outType == 0)
		outType = 'TRWP';
	if (!((outType == 'TEXT') || (outType == 'CSTR') || (outType == 'TRWP'))) {
		return B_NO_TRANSLATOR;
	}
	if (outType == 'TRWP') {
		if (inInfo == NULL)
			goto is_plain_text;
		if ((strncmp("text",inInfo->MIME,4) == 0) || ((inInfo->type == 'TEXT') || (inInfo->type == 'CSTR') || (inInfo->type == B_TRANSLATOR_TEXT))) {
		  is_plain_text:
			char *buffer;
			inSource->Seek(0,SEEK_END);
			off_t size = inSource->Position();
			buffer = new char[size];
			inSource->Seek(0,SEEK_SET);
			inSource->Read(buffer,size_t(size));
			//---------Find foreign line breaks------------------
			BString editing(buffer);
			delete buffer;
			//---------Get Windows breaks------------------------
			editing.ReplaceAll("\r\n","\n");
			//---------Zap Mac breaks----------------------------
			editing.ReplaceAll('\r','\n');
			//---------Make a TranslatorWP
			inSource->Seek(0,SEEK_SET);
			TranslatorWP transWP(editing.String());
			if (is_kind_of(inSource,BNode))
				pullSEInfo((BNode *)inSource,&transWP,size);
			BMessage msg;
			transWP.Archive(&msg);
			msg.Flatten(outDestination);
			return B_OK;
		} else
			return B_NO_TRANSLATOR;
	} else {
		BMessage msg;
		TranslatorWP *transWP;
		if (msg.Unflatten(inSource) != B_OK)
			return B_NO_TRANSLATOR;
		if (validate_instantiation(&msg,"TranslatorWP") == false)
			return B_NO_TRANSLATOR;
		transWP = (TranslatorWP *)TranslatorWP::Instantiate(&msg);
		if ((transWP == NULL) || (!is_kind_of(transWP,TranslatorWP)))
			return B_NO_TRANSLATOR;
		outDestination->Write(transWP->Text(),strlen(transWP->Text()));
		delete transWP;
		return B_OK;
	}
	return B_NO_TRANSLATOR;
}

void pullSEInfo(BNode *node,TranslatorWP *WP,size_t length) {
	//------------Incomplete, GetAttrInfo goes screwy---------------
	attr_info bogus;
	if (node->InitCheck() != B_OK)
		return;
	if (node->GetAttrInfo("alignment",&bogus) == B_OK) {
		int32 align;
		node->ReadAttr("alignment",'LONG',0,&align,4);
		switch (align) {
			case B_ALIGN_LEFT:
				WP->pushAttr(0,length,justification,"left",5);
				break;
			case B_ALIGN_CENTER:
				WP->pushAttr(0,length,justification,"center",7);
				break;
			case B_ALIGN_RIGHT:
				WP->pushAttr(0,length,justification,"right",6);
				break;
		}
	}
}