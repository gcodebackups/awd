#ifndef _LIBAWD_ATTR_H
#define _LIBAWD_ATTR_H

#include "awd_types.h"

#define ATTR_RETURN_NULL AWD_attr_val_ptr _ptr; _ptr.str = NULL; return _ptr;

typedef awd_uint16 awd_propkey;

typedef enum {
    AWD_ATTR_INT16=1,
    AWD_ATTR_INT32,
    AWD_ATTR_FLOAT32,
    AWD_ATTR_FLOAT64,
    AWD_ATTR_STRING,
    AWD_ATTR_BADDR,
    AWD_ATTR_MTX4,
} AWD_attr_type;


typedef union {
    void *v;
    awd_int16 *i16;
    awd_int32 *i32;
    awd_float32 *f32;
    awd_float64 *f64;
    awd_baddr *addr;
    char *str;
} AWD_attr_val_ptr;



class AWDAttr
{
    protected:
        virtual void write_metadata(int)=0;

    public:
        AWD_attr_type type;
        AWD_attr_val_ptr value;
        awd_uint16 value_len;

        void write_attr(int, bool, bool);
};


/**
 * User attributes.
*/
class AWDUserAttr : public AWDAttr
{
    protected:
        void write_metadata(int);

    public:
        char *key;
        awd_uint16 key_len;
        awd_uint8 ns_addr;

        AWDUserAttr *next;

        AWDUserAttr();
        ~AWDUserAttr();
};


class AWDUserAttrList {
    private:
        AWDUserAttr *first_attr;
        AWDUserAttr *last_attr;

        AWDUserAttr *find(char *, awd_uint16);

    public:
        AWDUserAttrList();
        ~AWDUserAttrList();

        awd_uint32 calc_length(bool, bool);
        void write_attributes(int, bool, bool);

        AWD_attr_val_ptr get(char *, awd_uint16);
        void set(char *, awd_uint16, AWD_attr_val_ptr, awd_uint16, AWD_attr_type);
};



/**
 * Numeric attributes ("properties")
*/
class AWDNumAttr : public AWDAttr
{
    protected:
        void write_metadata(int);

    public:
        awd_propkey key;

        AWDNumAttr *next;

        AWDNumAttr();
};


class AWDNumAttrList {
    private:
        AWDNumAttr *first_attr;
        AWDNumAttr *last_attr;

        AWDNumAttr *find(awd_propkey);

    public:
        AWDNumAttrList();
        ~AWDNumAttrList();
        awd_uint32 calc_length(bool, bool);
        void write_attributes(int, bool, bool);

        AWD_attr_val_ptr get(awd_propkey);
        void set(awd_propkey, AWD_attr_val_ptr, awd_uint16, AWD_attr_type);
};




/**
 * "Attribute element", any element (such as a block, or parts of block
 * like skeleton joints) which can have attributes.
*/
class AWDAttrElement
{
    protected:
        AWDAttrElement();
        ~AWDAttrElement();

        AWDNumAttrList *properties;
        AWDUserAttrList *user_attributes;

        awd_uint32 calc_attr_length(bool, bool, bool, bool);
};

#endif
