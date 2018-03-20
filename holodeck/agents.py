import numpy as np


class HolodeckAgent(object):
    def __init__(self, client, name="DefaultAgent"):
        self.name = name
        self._client = client
        self._action_buffer, self._teleport_bool_buffer, self._teleport_buffer = \
            self._client.subscribe_command(name, self.__action_space_shape__())

    def act(self, action):
        self.__act__(action)

    def teleport(self, location):
        # The default teleport function is to copy the data to the buffer and set the bool to true
        # It can be overridden if needs be.
        np.copyto(self._teleport_buffer, location)
        np.copyto(self._teleport_bool_buffer, True)

    @property
    def action_space(self):
        raise NotImplementedError()

    def __action_space_shape__(self):
        raise NotImplementedError()

    def __act__(self, action):
        # The default act function is to copy the data,
        # but if needed it can be overridden
        np.copyto(self._action_buffer, action)

    def __repr__(self):
        return "HolodeckAgent"


class UAVAgent(HolodeckAgent):
    @property
    def action_space(self):
        # TODO(joshgreaves) : Remove dependency on gym
        # return spaces.Box(-1, 3.5, shape=[4])
        pass

    def __action_space_shape__(self):
        return [4]

    def __repr__(self):
        return "UAVAgent"


class ContinuousSphereAgent(HolodeckAgent):
    @property
    def action_space(self):
        # TODO(joshgreaves) : Remove dependency on gym
        # return spaces.Box(np.array([-1, -.25]), np.array([1, .25]))
        pass

    def __action_space_shape__(self):
        return [2]

    def __repr__(self):
        return "ContinuousSphereAgent"


class DiscreteSphereAgent(HolodeckAgent):
    @property
    def action_space(self):
        # TODO(joshgreaves) : Remove dependency on gym
        # return spaces.Discrete(4)
        pass

    def __action_space_shape__(self):
        return [2]

    def __act__(self, action):
        actions = np.array([[2, 0], [-2, 0], [0, 2], [0, -2]])
        to_act = np.array(actions[action, :])

        np.copyto(self._action_buffer, to_act)

    def __repr__(self):
        return "DiscreteSphereAgent"


class AndroidAgent(HolodeckAgent):
    @property
    def action_space(self):
        # TODO(joshgreaves) : Remove dependency on gym
        # return spaces.Box(-1000, 1000, shape=[127])
        pass

    def __action_space_shape__(self):
        return [127]

    def __repr__(self):
        return "AndroidAgent"


class NavAgent(HolodeckAgent):
    @property
    def action_space(self):
        # TODO(joshgreaves) : Remove dependency on gym
        # return spaces.Box(-10000, 10000, shape=[3])
        pass

    def __action_space_shape__(self):
        return [3]

    def __repr__(self):
        return "NavAgent"
