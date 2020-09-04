pipeline {
  agent { 
    dockerfile {
      args '--runtime=nvidia'
    }
  }
  stages {
    stage('build') {
      steps {
	sh 'whoami'
	sh 'ls ~/.local/share/holodeck/0.3.1/worlds/'
        sh '''
          pip install .
        '''
      }
    }
    stage('test') {
      steps {
        sh '''
	  su - holodeckuser
          py.test
        '''
      }
    }

  }
}
