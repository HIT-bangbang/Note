while(1)
所有环境的step+1
    for i in length(env_nums)
        if step[i]==falldown_episode_length   给螺栓下落的时间
            更新图片（当前状态）
            获得new_goal = model(img)
            爪子张开距离
            set target = 要到达的goal
        if step[i] == reach_episode_length 到达目标的位置
            合并爪子
        if step[i] == grasp_episode_length 爪子合并完毕
            抬起来
        if step[i]>max_episode_length-1 到达最终步长，已经拣起来了
            计算奖励
            获得新的图片（新的状态）
            进行学习
            reset这个环境（螺栓重置，机械臂复位，step归0）
        

