source scripts/lib.gdb

define show_task_grp
	if $argc != 1
		help show_task_grp
	else
		set print pretty on
		set $grp = (task_group_t *)$arg0

		# print group info
		printf "Group ID : %d\n",$grp->group_id
		print *(task_group_t *)$grp
		printf "&task_list: 0x%08x\n", &$grp->task_list
		printf "---Tasks--\n"
		dump_list &($grp->task_list) task_t task_list
	end
end

document show_task_grp
	help print task group verbosely
	usage: show_task_grp <task_group_t *>
end


define show_task_thr
	if $argc != 1
		help show_task_thr
	else
		set print pretty on
                set $tsk = (task_t *)$arg0
		set $cur = $tsk

		# print task info
		print *(task_t *)$tsk
		printf "--Threads--\n"
		dump_list &($tsk->thread_list) thread_t thread_list
	end
end

document show_task_thr
	help print threads
	usage: show_task_thr <task_t *>
end


define show_rq
	if $argc != 1
		help show_rq
	else
		set print pretty on
		set $cpu = (cpu_state_t *)$arg0
		dump_list &($cpu->runq) rthread_list_t runq
	end
end

document show_rq
	help print run-queue of specified cpu
	usage: show_rq <cpu_state_t *>
end
