# offsetof <type> <member>
# $rc - offset of the <member> in struct of <type>
define offsetof
	set $rc = (unsigned int)&(($arg0 *)0)->$arg1
end

# list_entry <list_head addr> <struct type> <list_head member>
# $rc - initial address of struct with given <struct type>
define list_entry
	offsetof $arg1 $arg2
	set $rc = ($arg1 *)((unsigned int) ($arg0) - $rc)
end

define dump_list
    set $list = $arg0
    set $e = $list->next
    set $i = 1
    while $e != $list
        list_entry $e $arg1 $arg2
        set $l = $rc
        printf "list_head: dumplist #%d: %p ", $i++, $l
        output *$l
        set $e = $e->next
        printf "\n"
    end
end

document dump_list
        print list_head 
        usage dump_list <head> <type> <list_head member>
end

