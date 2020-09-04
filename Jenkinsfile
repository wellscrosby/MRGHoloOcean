pipeline {
  agent { 
    dockerfile {
      args '--runtime=nvidia'
    }
  }
  stages {
    stage('build') {
      steps {
        sh '''
          pip install .
        '''
      }
    }
    stage('test') {
      steps {
        sh '''
          su - holodeckuser
	  whoami
          ls /home/holodeckuser/.local/share/holodeck/0.3.1/worlds/
          py.test
        '''
      }
    }

  }
}
