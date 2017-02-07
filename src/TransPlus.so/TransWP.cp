class __declspec(dllexport) TranslatorWP;

#include <Sound.h>
#include <List.h>
#include <Message.h>
#include <String.h>
#include <Picture.h>
#include <TranslationUtils.h>
#include <BitmapStream.h>
#include <Bitmap.h>

#include "TransWP.h"

//----------------List functions------------------------------------------

class embedded_list : public BList {
	friend class TranslatorWP; //------------No one but TransWP needs to see this-------
	protected:
		//
		typedef struct {
			int32 offset;
			void *data;
			size_t size;
			uint32 type;
		} embedded_offset;
		//
		bool AddItem(int32 offset, const void *data, size_t size, uint32 type) {
			embedded_offset *to_add = new embedded_offset;
			to_add->offset = offset; to_add->data = new unsigned char[size];
			memcpy(to_add->data,data,size); to_add->size = size; to_add->type = type;
			bool result = BList::AddItem(to_add);
			SortItems();
			return result;
		}
		void SortItems(void) {
			embedded_offset *attr;
			if (CountItems() < 2)
				return;
			for (int32 i = CountItems() - 1; i >= 0; i--) {
				attr = (embedded_offset *)(BList::ItemAt(i));
				MoveItem(i,IndexOfOffset(attr->offset));
			}
		}
		int32 IndexOfOffset(int32 offset) {
			embedded_offset *cur;
			for (int32 i = 0; i < CountItems(); i++) {
				cur = (embedded_offset *)BList::ItemAt(i);
				if (cur->offset == offset)
					return i;
			}
			return -1;
		}
		embedded_offset *ItemAt(int32 offset,int32 index) {
			embedded_offset *cur;
			int32 i = IndexOfOffset(offset);
			i += index;
			cur = (embedded_offset *)(BList::ItemAt(i));
			if (cur == NULL)
				return NULL;
			if (cur->offset != offset)
				return NULL;
			return cur;
		}
};

class attr_list : public BList { 
	friend class TranslatorWP; //------------No one but TransWP needs to see this-------
	protected:
		//
		typedef struct {
			int32 offset;
			WP_Attr *attr;
		} attr_offset;
		//
		attr_list(void) : BList() {}
		bool AddItem(int32 offset, WP_Attr *attr) {
			attr_offset *to_add = new attr_offset;
			to_add->offset = offset; to_add->attr = attr;
			bool result = BList::AddItem(to_add);
			SortItems();
			return result;
		}
		void SortItems(void) {
			attr_offset *attr;
			if (CountItems() < 2)
				return;
			for (int32 i = CountItems() - 1; i >= 0; i--) {
				attr = (attr_offset *)(BList::ItemAt(i));
				MoveItem(i,IndexOfOffset(attr->offset));
			}
		}
		int32 OffsetOf(WP_Attr *attr) {
			attr_offset *cur;
			for (int32 i = 0; i < CountItems(); i++) {
				cur = (attr_offset *)BList::ItemAt(i);
				if (cur->attr == attr)
					return cur->offset;
			}
			return -1;
		}
		int32 IndexOfOffset(int32 offset) {
			attr_offset *cur;
			for (int32 i = 0; i < CountItems(); i++) {
				cur = (attr_offset *)BList::ItemAt(i);
				if (cur->offset == offset)
					return i;
			}
			return -1;
		}
		WP_Attr *ItemAt(int32 offset,int32 index) {
			attr_offset *cur;
			int32 i = IndexOfOffset(offset);
			i += index;
			cur = (attr_offset *)(BList::ItemAt(i));
			if (cur == NULL)
				return NULL;
			if (cur->offset != offset)
				return NULL;
			return cur->attr;
		}
};

//----------------------Begin TransWP Functions-----------------------------
TranslatorWP::TranslatorWP(void) : BArchivable() {
	begin_attrs = new attr_list;
	end_attrs = new attr_list;
	embedded = new embedded_list;
}

TranslatorWP::TranslatorWP(const char *plainText) : BArchivable() {
	begin_attrs = new attr_list;
	end_attrs = new attr_list;
	embedded = new embedded_list;
	SetText(plainText);
}

TranslatorWP::TranslatorWP(BMessage *archive) : BArchivable(archive) {
	begin_attrs = new attr_list;
	end_attrs = new attr_list;
	embedded = new embedded_list;
	SetText(archive->FindString("text"));
	int32 count;
	type_code type;
	archive->GetInfo("attr",&type,&count);
	//----Stuff we'll need-------------------
	const void* buffer;
	uint32 attr_type;
	int32 begin,end;
	ssize_t size;
	size_t data_size;
	//---------------------------------------
	for (int32 i = 0; i < count; i++) {
		archive->FindData("attr",type,i,&buffer,&size);
		memcpy(&begin,buffer,4);
		memcpy(&end,(void *)((addr_t)buffer + 4),4);
		memcpy(&attr_type,(void *)((addr_t)buffer + 8),4);
		memcpy(&data_size,(void *)((addr_t)buffer + 12),sizeof(size_t));
		pushAttr(begin,end,attr_type,(void *)((addr_t)buffer + 12 + sizeof(size_t)),data_size);
	}
	//---------------------------------------
	archive->GetInfo("embedded",&type,&count);
	for (int32 i = 0; i < count; i++) {
		archive->FindData("embedded",type,i,&buffer,&size);
		memcpy(&begin,buffer,4);
		memcpy(&attr_type,(void *)((addr_t)buffer + 4),4);
		memcpy(&data_size,(void *)((addr_t)buffer + 8),sizeof(size_t));
		AddEmbedded(attr_type,begin,(void *)((addr_t)buffer + 12),data_size);
	}
	archive->FindMessage("global",&globals);
}

void TranslatorWP::SetText(const char *plainText) {
	text.SetTo(plainText);
}

const char *TranslatorWP::Text(void) {
	return text.String();
}

void TranslatorWP::pushAttr(int32 begin, int32 end, uint32 type, const void *data, size_t size) {
	WP_Attr *attr = new WP_Attr;
	attr->attr_type = type; attr->size = size;
	attr->data = new unsigned char[size];
	memcpy((void *)(attr->data),(void *)data,size);
	begin_attrs->AddItem(begin,attr);
	end_attrs->AddItem(end,attr);
}

void TranslatorWP::pushAttr(int32 begin, int32 end, WP_Attr *attr) {
	begin_attrs->AddItem(begin,attr);
	end_attrs->AddItem(end,attr);
}

void TranslatorWP::pushAttr(int32 begin, int32 end, uint32 type, const char *data) {
	pushAttr(begin,end,type,data,strlen(data) + 1);
}

WP_Attr *TranslatorWP::getAttr(int32 begin, int32 *end, int32 index) {
	WP_Attr *attr = begin_attrs->ItemAt(begin,index);
	*end = end_attrs->OffsetOf(attr);
	return attr;
}

WP_Attr *TranslatorWP::getAttr(int32 *begin, int32 end, int32 index) {
	WP_Attr *attr = end_attrs->ItemAt(end,index);
	*begin = begin_attrs->OffsetOf(attr);
	return attr;
}

void TranslatorWP::AddEmbedded(uint32 data_type /*--See Translator Groups--*/,int32 offset,const void *data,size_t size) {
	embedded->AddItem(offset,data,size,data_type);
}

status_t TranslatorWP::GetEmbedded(uint32 *data_type /*--See Translator Groups--*/,int32 offset,const void **data,size_t *size,int32 index) {
	embedded_list::embedded_offset *callisto = embedded->ItemAt(offset,index);
	if (callisto == NULL)
		return B_BAD_VALUE;
	*data_type = callisto->type;
	*data = callisto->data;
	*size = callisto->size;
	return B_OK;
}

status_t TranslatorWP::GetEmbedded(uint32 data_type /*--See Translator Groups--*/,int32 offset,const void **data,size_t *size,int32 index) {
	int32 titan = -1;
	embedded_list::embedded_offset *deimos;
	for (int32 i = 0; (deimos = embedded->ItemAt(offset,i)) != NULL; i++) {
		if (deimos->type == data_type)
			titan++;
		if (titan == index)
			break;
	}
	if (titan != index)
		return B_BAD_VALUE;
	*data = deimos->data;
	*size = deimos->size;
	return B_OK;
}

void TranslatorWP::AddPicture(int32 offset,BPicture *picture) {
	BMessage msg;
	BMallocIO io;
	picture->Archive(&msg);
	msg.Flatten(&io);
	AddEmbedded(B_TRANSLATOR_PICTURE,offset,io.Buffer(),io.BufferLength());
}

void TranslatorWP::AddPicture(int32 offset,BBitmap *bitmap) {
	BBitmapStream ganymede(bitmap);
	unsigned char *io = new unsigned char[ganymede.Size()];
	size_t europa = ganymede.Size();
	ganymede.ReadAt(0,io,europa);
	memcpy((void *)(addr_t(io) + sizeof(TranslatorBitmap)),bitmap->Bits(),europa - sizeof(TranslatorBitmap));
	AddEmbedded(B_TRANSLATOR_BITMAP,offset,io,europa);
	delete io;
}

void TranslatorWP::AddPicture(int32 offset,const void *translatorBitmap,size_t size) {
	AddEmbedded(B_TRANSLATOR_BITMAP,offset,translatorBitmap,size);
}

BPicture *TranslatorWP::GetPicture(int32 offset,int32 index) {
	BMessage luna;
	const void *phobos;
	size_t sol;
	if (GetEmbedded(B_TRANSLATOR_PICTURE,offset,&phobos,&sol,index) != B_OK)
		return NULL;
	BMemoryIO jove(phobos,sol);
	luna.Unflatten(&jove);
	return ((BPicture *)BPicture::Instantiate(&luna));
}	
	
BBitmap *TranslatorWP::GetBitmap(int32 offset,int32 index) {
	const void *phobos;
	size_t sol;
	if (GetEmbedded(B_TRANSLATOR_BITMAP,offset,&phobos,&sol,index) != B_OK)
		return NULL;
	BMemoryIO jove(phobos,sol);
	return (BTranslationUtils::GetBitmap(&jove));
}

void TranslatorWP::AddEmbeddedText(int32 offset,const char *text) {
	AddEmbedded('TEXT',offset,text,strlen(text) + 1);
}

void TranslatorWP::AddEmbeddedText(int32 offset,TranslatorWP *formatted) {
	BMessage cygni;
	BMallocIO centauris;
	formatted->Archive(&cygni);
	cygni.Flatten(&centauris);
	AddEmbedded('TRWP',offset,centauris.Buffer(),centauris.BufferLength());
}

const char *TranslatorWP::GetEmbeddedText(int32 offset, int32 index) {
	size_t mars;
	const char *venus;
	if (GetEmbedded('TEXT',offset,(const void **)&venus,&mars,index) != B_OK)
		return NULL;
	return venus;
}

TranslatorWP *TranslatorWP::GetEmbeddedFormattedText(int32 offset, int32 index) {
	size_t mars;
	const char *venus;
	BMessage mercury;
	if (GetEmbedded('TRWP',offset,(const void **)&venus,&mars,index) != B_OK)
		return NULL;
	BMemoryIO earth(venus,mars);
	mercury.Unflatten(&earth);
	return (TranslatorWP *)Instantiate(&mercury);
}

void TranslatorWP::AddSound(int32 offset,BSound *sound) {
	//-------------------I don't know how to do this-------------------
}

void TranslatorWP::AddSound(int32 offset,const void *translatorSound,size_t size) {
	AddEmbedded(B_TRANSLATOR_SOUND,offset,translatorSound,size);
}

status_t TranslatorWP::GetSound(int32 offset,BSound *sound, int32 index) {
	//-------------------I don't know how to do this-------------------
	return B_ERROR;
}

void TranslatorWP::pushGlobal(uint32 type,const char *name,const void *data,size_t size) {
	globals.AddData(name,type,data,size,false);
}

void TranslatorWP::pushGlobal(const char *data,const char *name) {
	globals.AddString(name,data);
}

const void *TranslatorWP::getGlobal(int32 index,uint32 type,size_t *size,char **name) {
	const void *sirius;
	ssize_t the_dog_star;
	globals.GetInfo(type,index,name,&type);
	globals.FindData(*name,type,&sirius,&the_dog_star);
	*size = the_dog_star;
	return sirius;
}

const char *TranslatorWP::getGlobal(int32 index,char **name) {
	size_t rhea;
	return (const char *)(getGlobal(index,'CSTR',&rhea,name));
}

const void *TranslatorWP::getGlobal(const char *name,uint32 type,size_t *size,int32 index) {
	const void *tethys;
	ssize_t solaria;
	globals.FindData(name,type,index,&tethys,&solaria);
	*size = solaria;
	return tethys;
}

const char *TranslatorWP::getGlobal(const char *name,int32 index) {
	const char *aurora;
	globals.FindString(name,index,&aurora);
	return aurora;
}

status_t TranslatorWP::Archive(BMessage *archive,bool deep) {
	status_t result = BArchivable::Archive(archive,deep);
	if (result != B_OK)
		return result;
	int32 count = embedded->CountItems();
	archive->AddString("text",text.String());
	//----Stuff we'll need-------------------
	unsigned char* buffer;
	WP_Attr *c;
	int32 d;
	attr_list::attr_offset *b;
	embedded_list::embedded_offset *e;
	//---------------------------------------
	for (int32 i = 0; i < count; i++) {
		e = (embedded_list::embedded_offset *)embedded->BList::ItemAt(i);
		buffer = new unsigned char[12 + e->size];
		memcpy(buffer,&(e->offset),4);
		memcpy(buffer + 4,&(e->type),4);
		memcpy(buffer + 8,&(e->size),4);
		memcpy(buffer + 12,e->data,e->size);
		archive->AddData("embedded",'WPTR',buffer,12 + c->size,false);
		delete buffer;
	}
	//---------------------------------------
	count = begin_attrs->CountItems();
	for (int32 i = 0; i < count; i++) {
		b = (attr_list::attr_offset *)begin_attrs->BList::ItemAt(i);
		c = b->attr;
		buffer = new unsigned char[12 + sizeof(size_t) + c->size];
		d = end_attrs->OffsetOf(c);
		memcpy(buffer,&(b->offset),4);
		memcpy(buffer + 4,&d,4);
		memcpy(buffer + 8,&(c->attr_type),4);
		memcpy(buffer + 12,&(c->size),sizeof(size_t));
		memcpy(buffer + 12 + sizeof(size_t),c->data,c->size);
		archive->AddData("attr",'WPTR',buffer,12 + sizeof(size_t) + c->size,false);
		delete buffer;
	}
	archive->AddMessage("global",&globals);
	return B_OK;
}

BArchivable *TranslatorWP::Instantiate(BMessage *archive) {
	if ( !validate_instantiation(archive, "TranslatorWP") ) 
		return NULL; 
	return new TranslatorWP(archive);
}

TranslatorWP::~TranslatorWP(void) {
	attr_list::attr_offset *b;
	embedded_list::embedded_offset *c;
	for (int32 i = 0; i < begin_attrs->CountItems(); i++) {
		b = (attr_list::attr_offset *)begin_attrs->BList::ItemAt(i);
		delete b->attr->data;
		delete b->attr;
		delete b;
	}
	for (int32 i = 0; i < embedded->CountItems(); i++) {
		c = (embedded_list::embedded_offset *)embedded->BList::ItemAt(i);
		delete c->data;
		delete c;
	}
	delete begin_attrs;
	delete end_attrs;
	delete embedded;
}
