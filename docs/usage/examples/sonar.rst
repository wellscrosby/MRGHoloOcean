Visualizing Sonar Output
========================

It can be useful to visualize the output of the sonar sensor during a simulation. This script will do that, plotting each time sonar data is received.

::

    import holoocean
    import matplotlib.pyplot as plt
    import numpy as np

    #### GET SONAR CONFIG
    scenario = "OpenWater-HoveringSonar"
    config = holoocean.packagemanager.get_scenario(scenario)
    config = config['agents'][0]['sensors'][-1]["configuration"]
    azi = config['Azimuth']
    minR = config['MinRange']
    maxR = config['MaxRange']
    binsR = config['BinsRange']
    binsA = config['BinsAzimuth']

    #### GET PLOT READY
    plt.ion()
    fig, ax = plt.subplots(subplot_kw=dict(projection='polar'), figsize=(8,5))
    ax.set_theta_zero_location("N")
    ax.set_thetamin(-azi/2)
    ax.set_thetamax(azi/2)

    theta = np.linspace(-azi/2, azi/2, binsA)*np.pi/180
    r = np.linspace(minR, maxR, binsR)
    T, R = np.meshgrid(theta, r)
    z = np.zeros_like(T)

    plot = ax.pcolormesh(T, R, z, cmap='gray', shading='auto', vmin=0, vmax=1)
    plt.subplots_adjust(left=-.15, bottom=-.2, right=1.15, top=1.15)

    #### RUN SIMULATION
    command = np.array([0,0,0,0,-20,-20,-20,-20])
    with holoocean.make(scenario) as env:
        for i in range(1000):
            env.act("auv0", command)
            state = env.tick()

            if 'SonarSensor' in state:
                s = state['SonarSensor']
                plot.set_array(s.ravel())

                fig.canvas.draw()
                fig.canvas.flush_events()

    plt.ioff()   
    plt.show()