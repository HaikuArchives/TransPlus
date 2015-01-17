#ifndef _TRANS_WP_H

#define _TRANS_WP_H

enum { //-------------Common text attributes, use in WP_Attr::attr_type-----------
	font = 'FONT',
	color = 'COLR',
	size = 'SIZE',
	style = 'STYL',
	justification = 'JUST',
	spacing = 'SPCE',
	tab_width = 'TABS'
};

typedef struct {
	uint32 attr_type;
	const void *data;
	size_t size;
} WP_Attr;

class attr_list;
class embedded_list;

class TranslatorWP : public BArchivable { //----------Type Code is 'TRWP'---------
	public:
		TranslatorWP(void);
		TranslatorWP(const char *plainText);
		TranslatorWP(BMessage *archive);
		//
		void SetText(const char *plainText);
		const char *Text(void);
		//-----------Text Attributes-----------------------------------------------
		void pushAttr(int32 begin, int32 end, uint32 type, const void *data, size_t size);
		void pushAttr(int32 begin, int32 end, WP_Attr *attr);
		void pushAttr(int32 begin, int32 end, uint32 type, const char *data);
		WP_Attr *getAttr(int32 begin, int32 *end, int32 index = 0); //-----do NOT deallocate or modify----
		WP_Attr *getAttr(int32 *begin, int32 end, int32 index = 0); //-----do NOT deallocate or modify----
		//-----------Embedded Data--------------------------------------------------
			void AddEmbedded(uint32 data_type /*--See Translator Groups--*/,int32 offset,const void *data,size_t size);
			
			status_t GetEmbedded(uint32 *data_type /*--See Translator Groups--*/,int32 offset,const void **data,size_t *size, int32 index = 0);
			status_t GetEmbedded(uint32 data_type /*--See Translator Groups--*/,int32 offset,const void **data,size_t *size, int32 index = 0);
				//-------Pictures-------
			void AddPicture(int32 offset,BPicture *picture); //----Vector-------
			void AddPicture(int32 offset,BBitmap *bitmap);
			void AddPicture(int32 offset,const void *translatorBitmap,size_t size);
			
			BPicture *GetPicture(int32 offset, int32 index = 0); //----Vector-------
			BBitmap *GetBitmap(int32 offset,int32 index = 0);
				//-------Text-----------
			void AddEmbeddedText(int32 offset,const char *text);
			void AddEmbeddedText(int32 offset,TranslatorWP *formatted);
					//---Initialize all arguments
			const char *GetEmbeddedText(int32 offset, int32 index = 0);
			TranslatorWP *GetEmbeddedFormattedText(int32 offset,int32 index = 0);
				//-------Sounds---------
			void AddSound(int32 offset,BSound *sound);
			void AddSound(int32 offset,const void *translatorSound,size_t size);
					//---Initialize all arguments
			status_t GetSound(int32 offset,BSound *sound, int32 index = 0);
		//----------Gloabal Attributes----------------------------------------------
		void pushGlobal(uint32 type,const char *name,const void *data,size_t size);
		void pushGlobal(const char *data,const char *name);
		
		const void *getGlobal(int32 index,uint32 type,size_t *size,char **name);
		const char *getGlobal(int32 index,char **name);
		const void *getGlobal(const char *name,uint32 type,size_t *size,int32 index = 0);
		const char *getGlobal(const char *name,int32 index = 0);
		//-----------Archival-------------------------------------------------------
		status_t Archive(BMessage *archive,bool deep = true);
		static BArchivable *Instantiate(BMessage *archive);
		//
		~TranslatorWP(void);
	private:
		BString text;
		embedded_list *embedded;
		BMessage globals;
		attr_list *begin_attrs, *end_attrs;
};

#endif