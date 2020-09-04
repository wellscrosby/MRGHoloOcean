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
	sh 'echo ls'
        sh 'runuser -l holodeckuser -c \\"ls\\"'
        sh 'runuser -l holodeckuser -c \\"pytest\\"'
      }
    }

  }
}
