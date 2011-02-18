#ifndef _LIBAWD_SKELETON_H
#define _LIBAWD_SKELETON_H

#include "block.h"


class AWDSkeletonJoint
{
    private:
        awd_uint32 id;
        const char *name;
        awd_float64 *bind_mtx;

        AWDSkeletonJoint *parent;
        AWDSkeletonJoint *first_child;
        AWDSkeletonJoint *last_child;
        
    public:
        AWDSkeletonJoint *next;

        AWDSkeletonJoint(const char *, awd_float64 *);

        int write_joint(int, awd_uint32);
        int calc_length();

        awd_uint32 get_id();
        void set_parent(AWDSkeletonJoint *);
        AWDSkeletonJoint *get_parent();
        AWDSkeletonJoint *add_child_joint(AWDSkeletonJoint *);
};


class AWDSkeleton : public AWDBlock
{
    private:
        const char *name;
        AWDSkeletonJoint *root_joint;

    protected:
        awd_uint32 calc_body_length(awd_bool);
        void write_body(int, awd_bool);

    public:
        AWDSkeleton(const char *);

        AWDSkeletonJoint *set_root_joint(AWDSkeletonJoint *);
        AWDSkeletonJoint *get_root_joint();
};

#endif