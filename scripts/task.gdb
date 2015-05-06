define show_task_grp
	if $argc != 1
		help show_task_grp
	else
		set print pretty on
		set $grp = (task_group_t *)$arg0
		set $cur = $grp

		# print group info
		printf "Group ID : %d\n",$grp->group_id
		print *(task_group_t *)$grp
		printf "&task_list: 0x%08x\n", &$grp->task_list
		printf "---Tasks--\n"

		# 'cur' is either initial address of task_group_t or task_t
		set $cur = ((task_group_t *)$cur)->task_list
		while ((unsigned int)&($grp->task_list)) != (unsigned int)($cur.next)
			set $cur = $cur.next
			set $struct = (unsigned int)$cur - (unsigned int)&(((task_t *)0)->task_list)
			printf "&task_t : 0x%08x\n",(task_t *)$struct
			printf "&task_list : 0x%08x\n",$cur
			print *(task_t *)$struct
			printf "-----\n"
			set $cur = ((task_t *)$struct)->task_list
		end
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

		set $cur = ((task_t *)$cur)->thread_list
		while ((unsigned int)&($tsk->thread_list) != (unsigned int)($cur.next))
			set $cur = $cur.next
			set $thr = (unsigned int)$cur - (unsigned int)&(((thread_t *)0)->thread_list)
			printf "&thread_t    : 0x%08x\n",(thread_t *)$thr
			printf "&thread_list : 0x%08x\n",$cur
			print *(thread_t *)$thr
			printf "-----\n"
			set $cur = ((thread_t *)$thr)->thread_list
		end
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

		set $cur = $cpu->runq
		while ((unsigned int)&($cpu->runq) != (unsigned int)($cur.next))
			set $cur = $cur.next
			set $thread_list = (unsigned int)$cur - (unsigned int)&(((rthread_list_t *)0)->runq)
			print *(rthread_list_t *)$thread_list
			printf "-----\n"
			set $cur = ((rthread_list_t *)$thread_list)->runq
		end
	end
end

document show_rq
	help print run-queue of specified cpu
	usage: show_rq <cpu_state_t *>
end
