source scripts/lib.gdb

define show_mtab
        if $argc != 1
                help show_mtab
        else
                set print pretty on
                set $mtab = (mount_table_t *)$arg0
                dump_list &($mtab->mnt_next) mount_point_t mnt_next
        end
end

document show_mtab
        help print memory mount table
        usage: show_mtab <mount_table_t *>
end
