#include <ClassInfo.h>
#include <Sound.h>
#include <InterfaceDefs.h>
#include <image.h>
#include <String.h>
#include <Node.h>
#include <DataIO.h>
#include <Message.h>
#include <GraphicsDefs.h>
#include <TranslationDefs.h>
#include <TranslatorFormats.h>
#include <ctype.h>
#include <stdio.h>

extern "C" _EXPORT status_t Identify(BPositionIO *inSource,const translation_format *inFormat,
  BMessage *ioExtension,translator_info *outInfo, uint32 outType);

extern "C" _EXPORT status_t Translate(BPositionIO *inSource,const translator_info *inInfo,
  BMessage *ioExtension,uint32 outType,BPositionIO *outDestination);

extern "C" _EXPORT char translatorInfo[] = "HTML Translator, part of Translator Plus (http://transplus.sourceforge.net)";
extern "C" _EXPORT char translatorName[] = "HTML Translator";
extern "C" _EXPORT int32 translatorVersion = 100;

#include "TransWP.h"

#define baseTextSize 12

void ParseTags(TranslatorWP *work,BString HTML);
void pushAttrFromTag(TranslatorWP *work,BString tag /*No brackets*/,int32 begin,int32 end);
const char *pullFlag(BString tag, const char *flagName);
void ParseTags(BString *HTML,WP_Attr *attr,int32 offset,int32 *fromPlain,bool isEnd = false);

rgb_color parse_color(const char *string);
const char *parse_color(rgb_color color);

status_t Identify(BPositionIO *inSource,const translation_format *inFormat,
  BMessage *ioExtension,translator_info *outInfo, uint32 outType) {
		inSource->Seek(0,SEEK_END);
		off_t size = inSource->Position();
		char *document = new char[size + 1];
		inSource->Seek(0,SEEK_SET);
		inSource->Read(document,size_t(size));
		inSource->Seek(0,SEEK_SET);
		if (BString(document).IFindFirst("<HTML>") == B_ERROR) {
			delete [] document;
			BMessage msg;
			if (msg.Unflatten(inSource) == B_OK) {
				if (validate_instantiation(&msg,"TranslatorWP")) {
					outInfo->type = 'TRWP';
					outInfo->group = B_TRANSLATOR_TEXT;
					outInfo->quality = 1.0;
					outInfo->capability = 1.0;
					strcpy(outInfo->name,"TranslatorWP");
					strcpy(outInfo->MIME,"text/trans-WP");
					return B_OK;
				}
			}
			return B_NO_TRANSLATOR;
		}
		delete [] document;
		outInfo->type = 'HTML';
		outInfo->group = B_TRANSLATOR_TEXT;
		outInfo->quality = 1.0;
		outInfo->capability = 0.9;
		strcpy(outInfo->name,"HTML Document");
		strcpy(outInfo->MIME,"text/html");
		return B_OK;
}

status_t Translate(BPositionIO *inSource,const translator_info *inInfo,
  BMessage *ioExtension,uint32 outType,BPositionIO *outDestination) {
	BMessage message;
	bool notaTRWP = false;
	bool notHTML = false;
	inSource->Seek(0,SEEK_END);
	off_t size = inSource->Position();
	char *document = new char[size + 1];
	inSource->Seek(0,SEEK_SET);
	inSource->Read(document,size_t(size));
	inSource->Seek(0,SEEK_SET);
	if (BString(document).IFindFirst("<HTML>") == B_ERROR)
		notHTML = true;
	if (message.Unflatten(inSource) != B_OK)
		notaTRWP = true;
	delete [] document;
	if (notHTML && notaTRWP)
		return B_NO_TRANSLATOR;
	if (notaTRWP) {
		if ((outType != 'TRWP') && (outType != 0))
			return B_NO_TRANSLATOR;
		TranslatorWP work;
		inSource->Seek(0,SEEK_END);
		off_t size = inSource->Position();
		char *doc = new char[size + 1];
		inSource->Seek(0,SEEK_SET);
		inSource->Read(doc,size_t(size));
		doc[size] = 0;
		BString document(doc);
		delete [] doc;
		//------------Get rid of everything before <HTML>-----
		document.Remove(0,document.IFindFirst("<HTML>") + 6);
		//------------Get rid of everything after </HTML>-----
		document.Remove(document.IFindFirst("</HTML>"),document.Length() - document.IFindFirst("</HTML>"));
		//------------Fix Simple Formatting-------------------
		document.ReplaceAll("\n"," ");
		document.ReplaceAll("> <","><");
		document.IReplaceAll("<BR>","\n");
		document.IReplaceAll("<P>","\n\n");
		document.IReplaceAll("<LI>","\n• ");
		document.RemoveAll("</P>");
		document.RemoveAll("</p>");
		document.ReplaceAll("&nbsp;","\t");
		document.ReplaceAll("&quot;","\"");
		document.ReplaceAll("&amp;","&");
		document.ReplaceAll("&copy;",B_UTF8_COPYRIGHT);
		//------------Kill multiple spaces---------------------
		for (int32 i = 0; i <= document.Length();i++) {
			if (((document.ByteAt(i) == ' ') || (document.ByteAt(i) == '\t')) && ((document.ByteAt(i+1) == ' ') || (document.ByteAt(i+1) == '\t'))) {
				for (int32 a = i; a <= document.Length();a++) {
					if (!((document.ByteAt(a) == ' ') || (document.ByteAt(a) == '\t'))) {
						document.Remove(i + 1,a - (i+1));
						i--;
						break;
					}
				}
			}
		}
		document.ReplaceAll("> <","><");
		//------------Set plainText---------------------------
		//SetAsStripped(&work,document);
		//------------Do tags---------------------------------
		//new BApplication("application/trans");
		ParseTags(&work,document);
		//------------Archive It------------------------------
		BMessage msg;
		work.Archive(&msg);
		msg.Flatten(outDestination);
		return B_OK;
	} else {
		//-----------Unarchive it-----------------------------
		TranslatorWP *transWP;
		if (validate_instantiation(&message,"TranslatorWP") == false)
			return B_NO_TRANSLATOR;
		transWP = (TranslatorWP *)TranslatorWP::Instantiate(&message);
		if ((transWP == NULL) || (!is_kind_of(transWP,TranslatorWP)))
			return B_NO_TRANSLATOR;
		//-----------Set it up--------------------------------
		BString document(transWP->Text());
		//-----------Insert tags------------------------------
		int32 offsetFromPlain = 0;
		int32 temp;
		WP_Attr *cur;
		for (int32 i = 0; i < document.Length();i++) {
			for (int32 a = 0; (cur = transWP->getAttr(&temp,i,a)) != NULL; a++) {
				ParseTags(&document,cur,i,&offsetFromPlain,true);
			}
			for (int32 a = 0; (cur = transWP->getAttr(i,&temp,a)) != NULL; a++) {
				ParseTags(&document,cur,i,&offsetFromPlain,false);
			}
		}
		//------------Fix Simple Formatting-------------------
		document.ReplaceAll("\n• ","<LI>");
		document.ReplaceAll("\n\n\n","<P><BR>");
		document.ReplaceAll("\n\n","<P>");
		document.ReplaceAll("\n","<BR>");
		document.ReplaceAll("\t","&nbsp;");
		//document.ReplaceAll("\"","&quot;");
		//document.ReplaceAll("&","&amp;");
		document.ReplaceAll(B_UTF8_COPYRIGHT,"&copy;");
		BString beginning = "<HTML>\n";
		if (transWP->getGlobal("title") != NULL) {
			beginning << "<HEAD>\n<TITLE>";
			beginning << transWP->getGlobal("title");
			beginning << "</TITLE>\n</HEAD>\n";
		}
		size_t size;
		if (transWP->getGlobal("bkg_color",'RGBC',&size) != NULL) {
			beginning << "<BODY BGCOLOR=\"";
			beginning << parse_color(*((rgb_color *)(transWP->getGlobal("bkg_color",'RGBC',&size))));
			beginning << "\">\n";
		} else {
			beginning << "<BODY BGCOLOR=\"#ffffff\">\n";
		}
		document.Prepend(beginning);
		document.Append("</BODY>\n</HTML>");
		//-----------Write it out-----------------------------
		outDestination->Seek(0,SEEK_SET);
		outDestination->Write(document.String(),document.Length());
		//-----------Clean up---------------------------------
		delete transWP;
		return B_OK;
	}
	return B_NO_TRANSLATOR;
}

void ParseTags(BString *HTML,WP_Attr *attr,int32 offset,int32 *fromPlain,bool isEnd) {
	BString work;
	switch (attr->attr_type) {
		case font:
			if (isEnd) {
				HTML->Insert("</FONT>",offset + *fromPlain);
				*fromPlain += 7;
			} else {
				work = "<FONT FACE=\"\">";
				work.Insert((char *)(attr->data),12);
				HTML->Insert(work,offset + *fromPlain);
				*fromPlain += work.Length();
			}
			break;
		case color:
			if (isEnd) {
				HTML->Insert("</FONT>",offset + *fromPlain);
				*fromPlain += 7;
			} else {
				work = "<FONT COLOR=\"\">";
				work.Insert(parse_color(*((rgb_color *)(attr->data))),13);
				HTML->Insert(work,offset + *fromPlain);
				*fromPlain += work.Length();
			}
			break;
		case size:
			if (isEnd) {
				HTML->Insert("</FONT>",offset + *fromPlain);
				*fromPlain += 7;
			} else {
				float size = (*((float *)(attr->data)));
				work = "<FONT SIZE=";
				if (size < 15)
					work << int32(size - 12);
				else {
					work << "+";
					work << int32(size - 15);
				}
				work << ">";
				HTML->Insert(work,offset + *fromPlain);
				*fromPlain += work.Length();
			}
			break;
		case 'LINK':
			if (isEnd) {
				HTML->Insert("</A>",offset + *fromPlain);
				*fromPlain += 4;
			} else {
				work = "<A HREF=\"";
				work << (char *)(attr->data);
				work << "\">";
				HTML->Insert(work,offset + *fromPlain);
				*fromPlain += work.Length();
			}
			break;
		case 'ANCR':
			if (isEnd) {
				HTML->Insert("</A>",offset + *fromPlain);
				*fromPlain += 4;
			} else {
				work = "<A NAME=\"";
				work << (char *)(attr->data);
				work << "\">";
				HTML->Insert(work,offset + *fromPlain);
				*fromPlain += work.Length();
			}
			break;
		case style:
			//(new BAlert("ck",(char *)(attr->data),"OK"))->Go();
			if (strcmp((char *)(attr->data),"underline") == 0) {
				if (isEnd) {
					HTML->Insert("</U>",offset + *fromPlain);
					*fromPlain += 4;
				} else {
					HTML->Insert("<U>",offset + *fromPlain);
					*fromPlain += 3;
				}
				break;
			}
			if (strcmp((char *)(attr->data),"bold") == 0) {
				if (isEnd) {
					HTML->Insert("</B>",offset + *fromPlain);
					*fromPlain += 4;
				} else {
					HTML->Insert("<B>",offset + *fromPlain);
					*fromPlain += 3;
				}
				break;
			}
			if (strcmp((char *)(attr->data),"italic") == 0) {
				if (isEnd) {
					HTML->Insert("</I>",offset + *fromPlain);
					*fromPlain += 4;
				} else {
					HTML->Insert("<I>",offset + *fromPlain);
					*fromPlain += 3;
				}
				break;
			}
		default:
			if (((char *)attr->data)[attr->size - 1] == 0) {
				if (isEnd)
					work = "<";
				else
					work = "</";
				work << (char *)(attr->data);
				work << ">";
				work.ToUpper();
				HTML->Insert(work,offset + *fromPlain);
				*fromPlain += work.Length();
			}
			break;
	}
}

void ParseTags(TranslatorWP *work,BString HTML) {
	//----------------Kill the Title------------------
	if (HTML.IFindFirst("<TITLE>") != B_ERROR) {
		char title[1024];
		strncpy(title,HTML.String() + HTML.IFindFirst("<TITLE>") + 7,HTML.IFindFirst("</TITLE>") - (HTML.IFindFirst("<TITLE>") + 7));
		work->pushGlobal(title,"title");
		HTML.Remove(HTML.IFindFirst("<TITLE>"),HTML.IFindFirst("</TITLE>") + 8 - HTML.IFindFirst("<TITLE>"));
	}
	//----------------Get tags, parse them------------
	char curTag[1024];
	BString string,temp,tempHTML;
	int32 ending;
	size_t size;
	for (int32 i = 0; (i = HTML.FindFirst('<',i)) != B_ERROR; i++) {
		size = HTML.FindFirst('>',i) - (i + 1);
		strncpy(curTag,(char *)((addr_t)(HTML.String()) + i + 1),size);
		curTag[size] = 0;
		string.SetTo(curTag);
		if (HTML.ByteAt(i+1) == '/')
			continue;
		for (int32 a = 0;a <= strlen(curTag);a++) {
			if ((curTag[a] == ' ') || (curTag[a] == '\t')) {
				curTag[a] = 0;
				break;
			}
		}
		temp.SetTo(curTag);
		temp.Prepend("</");
		temp.Append(">");
		HTML.Remove(i,HTML.FindFirst('>',i) - i + 1);
		i--;
		tempHTML = (char *)((addr_t)HTML.String() + i);
		for (ending = 0;ending < tempHTML.IFindFirst(temp.String());ending++) {
			if (tempHTML.ByteAt(ending) == '<') {
				if (tempHTML.IFindFirst(curTag) == ending + 1) {
					tempHTML.Remove(tempHTML.IFindFirst(temp.String(),ending),temp.Length());
				}
				tempHTML.Remove(ending,(tempHTML.FindFirst('>',ending)) - ending + 1);
				ending--;
			}
		}
		ending += i;
		if (ending == i)
			continue;
		HTML.RemoveFirst(temp.String());
		pushAttrFromTag(work,string,i + 1,ending);
	}
	work->SetText(HTML.String());
}

void pushAttrFromTag(TranslatorWP *work,BString tag /*No brackets*/,int32 begin,int32 end) {
	if (tag == "CENTER") {
		work->pushAttr(begin,end,justification,"center");
		return;
	}
	if (tag == "B") {
		work->pushAttr(begin,end,style,"bold");
		return;
	}
	if (tag == "I") {
		work->pushAttr(begin,end,style,"italic");
		return;
	}
	if (tag == "U") {
		work->pushAttr(begin,end,style,"underline");
		return;
	}
	if (tag == "B") {
		work->pushAttr(begin,end,style,"bold");
		return;
	}
	//----------------------Parse FONT tag------------------
	if ((toupper(tag.ByteAt(0)) == 'F') && (toupper(tag.ByteAt(1)) == 'O') && (toupper(tag.ByteAt(2)) == 'N') && (toupper(tag.ByteAt(3)) == 'T')) {
		if (tag.IFindFirst("FACE=") != B_ERROR)
			work->pushAttr(begin,end,font,pullFlag(tag,"FACE"));
		if (tag.IFindFirst("SIZE=") != B_ERROR) {
			BString working = pullFlag(tag,"SIZE");
			float final_size;
			if (working.ByteAt(0) == '+') {
				sscanf(working.String(),"+%f",&final_size);
				final_size += baseTextSize;
				final_size += 3;
			} else {
				sscanf(working.String(),"%f",&final_size);
				final_size += baseTextSize;
			}
			work->pushAttr(begin,end,size,&final_size,sizeof(float));
		}
		if (tag.IFindFirst("COLOR=") != B_ERROR) {
			rgb_color rgb = parse_color(pullFlag(tag,"COLOR"));
			work->pushAttr(begin,end,color,&rgb,sizeof(rgb_color));
		}
	}
	//----------------Parse Anchors and Links-------------------------
	if ((toupper(tag.ByteAt(0)) == 'A') && (tag.ByteAt(1) == ' ')) {
		if (tag.IFindFirst("HREF=") != B_ERROR) {
			work->pushAttr(begin,end,'LINK',pullFlag(tag,"HREF"));
		}
		if (tag.IFindFirst("NAME=") != B_ERROR) {
			work->pushAttr(begin,end,'ANCR',pullFlag(tag,"NAME"));
		}
	}
	//----------------Parse BODY tag----------------------------------
	if (tag.ICompare("BODY ",5) == 0) {
		if (tag.IFindFirst("BGCOLOR=") != B_ERROR) {
			rgb_color background = parse_color(pullFlag(tag,"BGCOLOR"));
			work->pushGlobal('RGBC',"bkg_color",&background,sizeof(rgb_color));
		}
		if (tag.IFindFirst("TEXT=") != B_ERROR) {
			rgb_color background = parse_color(pullFlag(tag,"BGCOLOR"));
			work->pushAttr(0,strlen(work->Text()),color,&background,sizeof(rgb_color));
		}
	}
}

rgb_color parse_color(const char *string) {
	BString manip = string;
	manip.ToLower();
	string = manip.String();
	char hexd[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	rgb_color color;
	unsigned char byte[3];
	unsigned char high;
	for (int i =0;i<3;i++) {
		for (int a = 0; a < 16;a++) {
			if (hexd[a] == string[(i*2)+1]) {
				high = a;
				break;
			}
		}
		for (int a = 0; a < 16;a++) {
			if (hexd[a] == string[(i*2)+2]) {
				byte[i] = (high*16) + a;
				break;
			}
		}
	}
	color.red = byte[0]; color.green = byte[1]; color.blue = byte[2];
	return color;
}

const char *parse_color(rgb_color color) {
	char hexd[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	static char string[8];
	string[0] = '#';
	unsigned char byte[3] = {color.red,color.green,color.blue};
	unsigned char high;
	for (int i =0;i<3;i++) {
		high = byte[i]/16;
		string[(i*2)+1] = hexd[high];
		string[(i*2)+2] = hexd[byte[i]-(high*16)];
	}
	string[7] = 0;
	return string;
}

const char *pullFlag(BString tag, const char *flagName) {
	static BString string;
	BString flag(flagName);
	flag.Append("=");
	int32 end_of_flag;
	int32 where_are_we = tag.IFindFirst(flag) + flag.Length();
	if (tag.ByteAt(where_are_we) == '\"') {
		end_of_flag = tag.FindFirst('\"',where_are_we+1);
		where_are_we ++;
	} else {
		end_of_flag = tag.FindFirst(' ',where_are_we+1);
		if (end_of_flag == B_ERROR)
			end_of_flag = tag.Length();
	}
	tag.Remove(end_of_flag,tag.Length()-end_of_flag);
	tag.Remove(0,where_are_we);
	string.SetTo(tag);
	return string.String();
}
