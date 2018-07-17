#!/usr/local/cs/bin/python3
import sys

block_dict = {}

"""
key: inode number

value: class inode_info
"""
class inode_info:
    def __init__(self):
        self.link_cnt = 0
        self.real_ref_num = 0
        self.child_list = []
        self.parent = -1
        self.isinode = 0

reference_inode = {}
inode_dict_parents = {}
inode_dict = {}
bfree = []
ifree = []
total_block = 0
total_inode = 0
rsv_block = [0, 1, 2, 3, 4, 5, 6, 7, 64]
damage = False



def super_and_group(lines):
    global total_block
    global total_inode
    global bfree
    global ifree
    for line in lines:
        items = line.split(',')
        if items[0] == "SUPERBLOCK":
            total_block = int(items[1])
            total_inode = int(items[2])
        elif items[0] == "BFREE":
            bfree.append(int(items[1]))
        elif items[0] == "IFREE":
            ifree.append(int(items[1]))
        else:
            continue


def make_dict(lines):
    global total_inode, total_block, rsv_block, damage, reference_inode, inode_dict_parents,\
           inode_dict, block_dict, bfree, ifree
    for line in lines:
        items = line.split(',')
        type = items[0]
        if type == "INODE":
            i_num = int(items[1])
            if i_num not in inode_dict:
                inode_dict[i_num] = inode_info()
            inode_dict[i_num].isinode = 1
            inode_dict[i_num].link_cnt = int(items[6])
            if len(items) == 27:
                for i in range(12, 27):
                    b_num = int(items[i])
                    if b_num == 0: continue
                    if i == 24:
                        offset = 12
                        lvl = 1
                        b_type = "INDIRECT "
                    elif i == 25:
                        offset = 268
                        lvl = 2
                        b_type = "DOUBLE INDIRECT "
                    elif i == 26:
                        offset = 65804
                        lvl = 3
                        b_type = "TRIPLE INDIRECT "
                    else:
                        offset = 0
                        lvl = 0
                        b_type = ""
                    if b_num < 0 or b_num > total_block:
                        print("INVALID " + b_type + "BLOCK " + str(b_num) + " IN INODE " \
                              + str(i_num) + " AT OFFSET " + str(offset))
                        damage = True
                    elif b_num in rsv_block:
                        print("RESERVED " + b_type + "BLOCK " + str(b_num) + " IN INODE " \
                              + str(i_num) + " AT OFFSET " + str(offset))
                        damage = True
                    elif b_num not in block_dict:
                        block_dict[b_num] = [[i_num, offset, lvl]]
                    else:
                        #this_block = block_dict.get(b_num, [])
                        block_dict[b_num].append([i_num, offset, lvl])
        elif type == "DIRENT":
            i_num = int(items[3])
            p_num = int(items[1])
            fname = items[6]
            reference_inode[i_num] = fname
            if i_num <= 0 or i_num > total_inode:
                print("DIRECTORY INODE " + str(p_num) + " NAME " + fname[0:len(fname)-1] + \
                      " INVALID INODE " + str(i_num))
                damage = True
                continue


            if i_num not in inode_dict:
                inode_dict[i_num] = inode_info()
            inode_dict[i_num].parent = p_num
            inode_dict[i_num].real_ref_num += 1

            
            if fname[0:3] == "'.'" and p_num != i_num:
                print("DIRECTORY INODE " + str(p_num) + " NAME '.' LINK TO INODE " +\
                      str(i_num) + " SHOULD BE " + str(p_num))
                damage = True
            elif fname[0:3] == "'.'":
                continue
            elif fname[0:4] == "'..'":
                this_inode = inode_dict.get(p_num, inode_info())
                this_inode.parent = i_num
                inode_dict[p_num] = this_inode

                this_inode = inode_dict.get(i_num, inode_info())
                this_inode.child_list.append(p_num)
                inode_dict[i_num] = this_inode

                inode_dict_parents[p_num] = i_num

            else:
                this_inode = inode_dict.get(i_num, inode_info())
                this_inode.parent = p_num
                inode_dict[i_num] = this_inode

                this_inode = inode_dict.get(p_num, inode_info())
                this_inode.child_list.append(i_num)
                inode_dict[p_num] = this_inode

        elif type == "INDIRECT":
            b_num = int(items[5])
            i_num = int(items[1])
            lvl = int(items[2])
            if lvl == 1:
                b_type = "INDIRECT "
                offset = 12
            if lvl == 2:
                b_type = "DOUBLE INDIRECT"
                offset = 268
            if lvl == 3:
                b_type = "TRIPLE INDIRECT"
                offset = 65804
            if b_num < 0 or b_num > total_block:
                print("INVALID " + b_type + "BLOCK " + str(b_num) + " IN INODE " \
                      + str(i_num) + " AT OFFSET " + str(offset))
                damage = True
            elif b_num in rsv_block:
                print("RESERVED " + b_type + "BLOCK " + str(b_num) + " IN INODE " \
                      + str(i_num) + " AT OFFSET " + str(offset))
                damage = True
            elif b_num not in block_dict:
                block_dict[b_num] = [[i_num, offset, lvl]]
            else:
                #this_block = block_dict.get(b_num, [])
                block_dict[b_num].append([i_num, offset, lvl])


def block_audit():
    global total_inode, total_block, rsv_block, damage, reference_inode, inode_dict_parents,\
           inode_dict, block_dict, bfree, ifree
    for x in range(8, total_block + 1):
        if x not in bfree and x not in block_dict and x not in rsv_block:
            print("UNREFERENCED BLOCK " + str(x))
            damage = True
        elif x in bfree and x in block_dict:
            print("ALLOCATED BLOCK " + str(x) + " ON FREELIST")
            damage = True
    
    for block in block_dict:
        if len(block_dict[block]) > 1:
            damage = True
            for info in block_dict[block]:
                lvl = int(info[2])
                if lvl == 0:
                    slvl = ""
                elif lvl == 1:
                    slvl = " INDIRECT"
                elif lvl == 2:
                    slvl = " DOUBLE INDIRECT"
                else:
                    slvl = " TRIPLE INDIRECT"

                print("DUPLICATE" + slvl + " BLOCK " + str(block) + " IN INODE " + str(info[0]) + " AT OFFSET " + str(info[1]))

def inode_audit():
    global total_inode, total_block, rsv_block, damage, reference_inode, inode_dict_parents,\
           inode_dict, block_dict, bfree, ifree
    for x in range(1, total_inode + 1):
        if x not in ifree and x not in (1, 3, 4, 5, 6, 7, 8, 9, 10) and x not in inode_dict:
            print("UNALLOCATED INODE " + str(x) + " NOT ON FREELIST")
            damage = True
        elif x in ifree and x in inode_dict:
            if inode_dict[x].isinode == 1:
                print("ALLOCATED INODE " + str(x) + " ON FREELIST")
                damage = True


def directory_audit():
    global total_inode, total_block, rsv_block, damage, reference_inode, inode_dict_parents,\
           inode_dict, block_dict, bfree, ifree
    for pnt in inode_dict_parents:
        if pnt == 2 and inode_dict_parents[pnt] != 2:
            print("DIRECTORY INODE 2 NAME '..' LINK TO INODE " + str(inode_dict_parents[pnt]) + " SHOULD BE 2")
            damage = True
        elif pnt == 2:
            continue
        elif pnt not in inode_dict:
            print("DIRECTORY INODE " + str(inode_dict_parents[pnt]) + " NAME '..' LINK TO INODE " + str(pnt) + " SHOULD BE " + str(inode_dict_parents[pnt]))
            damage = True
        elif inode_dict[pnt].parent != inode_dict_parents[pnt]:
            print("DIRECTORY INODE " + str(pnt) + " NAME '..' LINK TO INODE " + str(pnt) + " SHOULD BE " + str(inode_dict[pnt].parent))
            damage = True

    for count in inode_dict:
        linkcnt = inode_dict[count].link_cnt
        links = inode_dict[count].real_ref_num
        if links != linkcnt and inode_dict[count].isinode == 1:
            print("INODE " + str(count) + " HAS " + str(links) + " LINKS BUT LINKCOUNT IS " + str(linkcnt))
            damage = True

    for ref in reference_inode:
        if ref in ifree and ref in inode_dict:
            if inode_dict[ref].isinode == 0:
                parent_inode = inode_dict[ref].parent
                directory_name = reference_inode[ref]
                print("DIRECTORY INODE " + str(parent_inode) + " NAME " + directory_name[0:len(directory_name)-1] +  " UNALLOCATED INODE " + str(ref))
                damage = True




if __name__ == "__main__":
    try:
        infile = open(sys.argv[1], "r")
    except:
        sys.stderr.write("File Open Error\n")
        exit(1)
    text = infile.readlines()
    super_and_group(text)
    make_dict(text)
    block_audit()
    inode_audit()
    directory_audit()
    if damage == True:
        exit(2)
    else:
        exit(0)


