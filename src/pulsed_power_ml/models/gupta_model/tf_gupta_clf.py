"""
This module implements the gupta model using the TensorFlow Python API in order to enable an export of
an instance of this model.
"""
from typing import Tuple

import tensorflow as tf
from tensorflow import keras

from src.pulsed_power_ml.models.gupta_model.gupta_utils import tf_switch_detected
from src.pulsed_power_ml.models.gupta_model.gupta_utils import tf_calculate_feature_vector


# Define some constants
N_PEAKS_MAX = 9
DATA_POINT_SIZE = 3 * 2**16 + 4


class TFGuptaClassifier(keras.Model):

    def __init__(self,
                 window_size: tf.Tensor = tf.constant(10, dtype=tf.int32),
                 step_size: tf.Tensor = tf.constant(1, dtype=tf.int32),
                 switch_threshold: tf.Tensor = tf.constant(55, dtype=tf.float32),
                 fft_size_real: tf.Tensor = tf.constant(2**16, dtype=tf.int32),
                 sample_rate: tf.Tensor = tf.constant(2_000_000, dtype=tf.int32),
                 n_known_appliances: tf.Tensor = tf.constant(10, dtype=tf.int32),
                 spectrum_type: tf.Tensor = tf.constant(2, dtype=tf.int32),
                 apparent_power_list: tf.Tensor = tf.constant(value=tf.zeros([10]),
                                                              dtype=tf.float32),
                 n_neighbors: tf.Tensor = tf.constant(3, dtype=tf.int32),
                 distance_threshold: tf.Tensor = tf.constant(10, dtype=tf.float32),
                 training_data_features: tf.Tensor = tf.constant(1, dtype=tf.float32),
                 training_data_labels: tf.Tensor = tf.constant(1, dtype=tf.float32),
                 verbose: tf.Tensor = tf.constant(False, dtype=tf.bool),
                 check_phy_condition: tf.Tensor = tf.constant(False, dtype=tf.bool),
                 apparent_power_threshold: tf.Tensor = tf.constant(0, dtype=tf.float32),
                 name="TFGuptaClassifier",
                 **kwargs
                 ) -> None:
        """
        Instantiation method for the Gupta classifier.

        Parameters
        ----------
        window_size
            Number of spectra for background_window, switching_window and classification_window.
            Default = 10.
        step_size
            Step size for rolling window approach. Default = 1.
        switch_threshold
            Threshold for the switch detection algorithm (in dBm).
        fft_size_real
            Number of points in the spectrum (original FFT size / 2).
        sample_rate
            Sample rate of the DAQ.
        n_known_appliances
            Number of known appliances.
        spectrum_type
            Determines which spectrum is used. 0: voltage, 1: current, 2: apparent power
        switching_offset
            Number of frames to skip after a switching event has been detected before classifying the event.
        n_peaks_max
            Max. number of peaks, which are used to calculate features.
        apparent_power_list
            Tensor containing the apparent power per application. The order of the values is the same as
            for the labels for the KNN classifier.
        n_neighbors
            Number of nearest neighbors which should be checked during the classification.
        knn_weights
            How to weight the distance of al neighbors.
        distance_threshold
            If distance to the nearest neighbor is above this threshold, an event is classified as "other"
        training_data_features
            Unscaled (!) features of the training data
        training_data_labels
            Corresponding labels for the provided training data
        verbose
            If True, increase verbosity.
        apparent_power_threshold
            Allowed apparent power range ratio of physical boundary condition. If 0, disable physical boundary condition feature
        """
        super(TFGuptaClassifier, self).__init__(name=name, **kwargs)

        # verbose flag
        self.verbose = verbose

        # Parameters
        self.fft_size_real = fft_size_real
        self.sample_rate = sample_rate
        self.n_known_appliances = n_known_appliances
        self.spectrum_type = spectrum_type
        self.n_peaks_max = tf.constant(N_PEAKS_MAX,
                                       dtype=tf.int32)        
        self.switch_threshold = switch_threshold
        self.check_phy_condition = check_phy_condition 
        self.apparent_power_threshold = apparent_power_threshold
           
        # kNN-Classifier
        self.n_neighbors = n_neighbors
        self.distance_threshold = distance_threshold

        # Containers
        self.current_state_vector = tf.Variable(tf.zeros(shape=(n_known_appliances + 1),
                                                         dtype=tf.float32),
                                                dtype=tf.float32,
                                                trainable=False,
                                                name='current_state_vector')

        # Container for rolling window
        self.window_size = window_size
        self.step_size = step_size
        self.window = tf.Variable(
            initial_value=tf.zeros(shape=(3 * self.window_size, self.fft_size_real),
                                   dtype=tf.float32),
            dtype=tf.float32,
            trainable=False,
            name='window'
        )
        self.n_frames_in_window = tf.Variable(initial_value=0,
                                              dtype=tf.int32,
                                              trainable=False,
                                              name='n_frames_in_window')
        
        self.switching_flag = tf.Variable(initial_value=False, 
                                          dtype=tf.bool,
                                          trainable=False,
                                          name='switching_flag')
        self.base_spectrum = tf.Variable(
            initial_value=tf.zeros(shape=self.fft_size_real,dtype=tf.float32),
            dtype=tf.float32,
            trainable=False,
            name='base_spectrum'
        )
        # Variables for physical boundary condition                                     
        self.check_power_diff_later = tf.Variable(initial_value=False, 
                                            dtype=tf.bool,
                                            trainable=False,
                                            name='check_power_diff_later')

        self.switched_on_appliance_index = tf.Variable(initial_value=-1,
                                                       dtype=tf.int32,
                                                       trainable=False,
                                                       name='switched_on_appliance_index')
                                                       
        self.switched_off_appliance_index = tf.Variable(initial_value=-1,
                                                        dtype=tf.int32,
                                                        trainable=False,  
                                                        name='switched_off_appliance_index')                                                
                                           
        self.power_before_switch = tf.Variable(initial_value=-1,
                                               dtype=tf.float32,
                                               trainable=False,
                                               name='power_before_switch')      
 
        self.power_after_switch = tf.Variable(initial_value=-1,
                                              dtype=tf.float32,
                                              trainable=False,
                                              name='power_after_switch')                                                 
                                                   
        self.power_window = tf.Variable(tf.zeros(shape=(5 * self.window_size), dtype=tf.float32),
                                        dtype=tf.float32,
                                        trainable=False,
                                        name='power_window')                       
 
        self.n_in_power_window = tf.Variable(initial_value=0,
                                             dtype=tf.int32,
                                             trainable=False,
                                             name='n_in_power_window')
        
        self.skip_power_max = tf.Variable(initial_value=(5 * window_size),
                                          dtype=tf.int32,
                                          trainable=False,
                                          name='skip_power_max')  
                                          
        self.skip_power_counter = tf.Variable(initial_value=0,
                                              dtype=tf.int32,
                                              trainable=False,
                                              name='skip_power_counter')
                                                            
        # Apparent Power data base
        self.apparent_power_list = apparent_power_list

        # Training data for KNN
        self.training_data_features_raw = training_data_features
        self.training_data_labels = training_data_labels

        self.scale_tensor = tf.Variable(initial_value=tf.ones(shape=(3 * self.n_peaks_max, 1), dtype=tf.float32),
                                        dtype=tf.float32,
                                        trainable=False,
                                        shape=(3 * self.n_peaks_max, 1))

        self.scale_tensor.assign(
            tf.reduce_max(
                input_tensor=tf.expand_dims(tf.math.abs(training_data_features), axis=-1),
                axis=0,
                keepdims=False
            )
        )

        self.training_data_features_scaled = tf.math.divide_no_nan(self.training_data_features_raw,
                                                                   self.scale_tensor[:, 0])

        self.feature_vector = tf.Variable(
            initial_value=tf.ones(shape=(3 * self.n_peaks_max, 1), dtype=tf.float32),
            dtype=tf.float32,
            trainable=False,
            shape=(3 * self.n_peaks_max, 1)
        )

        return

    @tf.function(
        input_signature=[tf.TensorSpec(shape=(3 * N_PEAKS_MAX, 1), dtype=tf.float32)]
    )
    def call_knn(self, input_tensor: tf.Tensor) -> Tuple[tf.Tensor, tf.Tensor]:
        """
        Method to use this model for inference.

        Parameters
        ----------
        input_tensor
            Tensor consisting of one feature vector.

        Returns
        -------
        distances, label
            distances to the nearest neighbors
            label: Tensor containing the classification result.
        """
        # Shape must be (n_features, 1) for both
        scaled_input = tf.math.divide_no_nan(input_tensor,
                                             self.scale_tensor)

        difference_vectors = tf.math.subtract(
            self.training_data_features_scaled,
            scaled_input[:, 0])

        distances = tf.norm(difference_vectors, axis=1)

        neg_k_smallest_distances, k_smallest_indices = tf.math.top_k(
            input=distances * -1,
            k=self.n_neighbors,
            name="FindKNearestNeighbors"
        )

        k_smallest_distances = tf.math.multiply(neg_k_smallest_distances,
                                                tf.constant(-1, dtype=tf.float32))

        label_tensor = tf.gather(
            params=self.training_data_labels,
            indices=k_smallest_indices
        )

        if tf.math.equal(tf.math.reduce_min(k_smallest_distances), tf.constant(0, dtype=tf.float32)):
            # if distance to one point is zero, all other points are ignored (similar to scikit-learn implementation)
            result_tensor = self.training_data_labels[k_smallest_indices[0]]

        else:
            # weight classes with distances (similar to scikit learns weights="distance" weighting)
            weighted_label_tensor = label_tensor / tf.expand_dims(k_smallest_distances,
                                                                  axis=-1)

            # Determine the class
            temp_a = tf.reduce_sum(weighted_label_tensor, axis=0)
            result_index = tf.argmax(temp_a)

            # Create result tensor (one-hot encoded w/ length=N+1)
            result_tensor = tf.one_hot(indices=result_index,
                                       depth=self.training_data_labels.shape[1])

        if self.verbose:
            tf.print('\n')
            tf.print('# +++++++++++++++++++++++++++++++++++++ In "call_knn()" +++++++++++++++++++++++++++++++++++++ #')
            tf.print('Feature vector "input" = ', input_tensor, summarize=-1)
            tf.print('distances = ', distances, summarize=-1)
            tf.print('k_smallest_distances = ', k_smallest_distances, summarize=-1)
            tf.print('result_tensor = ', result_tensor, summarize=-1)

        return k_smallest_distances, result_tensor

    @tf.function(
        input_signature=[tf.TensorSpec(shape=DATA_POINT_SIZE, dtype=tf.float32)]
    )
    def call(self, X: tf.Tensor) -> tf.Tensor:
        """
        Prediction method of the Gupta classifier.

        Parameters
        ----------
        X
            One data point containing spectra, power and phase data.

        Returns
        -------
        y
            state_vector containing the current estimated power for each appliance.
        """
        # If data point is all exactly -1, reset internal states
        if tf.math.reduce_all(tf.math.equal(x=X, y=tf.math.multiply(-1.0, tf.ones_like(X, dtype=tf.float32)))):
            self.reset_internal_states()
            return self.current_state_vector

        # #+++++++++++++++++++++++# #
        # #+++ Crop data point +++# #
        # #+++++++++++++++++++++++# #
        raw_spectrum, apparent_power = self.crop_data_point(X)
        spectrum = 10.0 * tf.math.log(raw_spectrum) / tf.math.log(10.0) + 30.0

        # #++++++++++++++++++++++++++++++++# #
        # #+++ Print out spectrum (dBm) +++# #
        # #++++++++++++++++++++++++++++++++# #
        if self.verbose:
            tf.print('\n')
            tf.print('# +++++++++++++++++++++++++++++++++++++ In "call()" +++++++++++++++++++++++++++++++++++++ #')
            top_k_values, top_k_indices = tf.math.top_k(input=spectrum, k=5)
            tf.print('Indices of top 5 entries in input (transformed) = ', top_k_indices, summarize=-1)
            tf.print('Values of top 5 entries in input (transformed) = ', top_k_values, summarize=-1)
            tf.print('Min entry in input = ', tf.math.reduce_min(spectrum))
            tf.print('Mean value of input = ', tf.math.reduce_mean(spectrum))
            tf.print('Number of entries in window = ', self.n_frames_in_window)
            tf.print('Max window size = ', self.window_size)
            tf.print('Step size for rolling window = ', self.step_size)

        if self.check_phy_condition:
            self.update_power_window(apparent_power)
            if self.check_power_diff_later:
                self.check_apparent_power_diff(current_apparent_power=apparent_power)

                
        # #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++# #
        # #+++ Check if there are enough frames in the current window +++# #
        # #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++# #
        self.update_window(spectrum)
        if tf.math.less(self.n_frames_in_window, self.window_size * 3):

            self.current_state_vector.assign(
                self.calculate_unknown_apparent_power(current_apparent_power=apparent_power)
            )
            return self.current_state_vector

        # ++++++++++++++++++++++++++++++++++# #
        # #+++ Switching Event Detection +++# #
        # #+++++++++++++++++++++++++++++++++# #

        # calculate mean background
        current_background = tf.math.reduce_mean(
            input_tensor=self.window[2 * self.window_size:],
            axis=0,
            name='calculate_mean_background'
        )

        # calculate difference spectrum for switch detection
        current_signal = tf.math.reduce_mean(
            input_tensor=self.window[self.window_size:2 * self.window_size],
            axis=0,
            name='calculate_mean_signal'
        )
        
        difference_spectrum = tf.math.subtract(
            x=current_signal,
            y=current_background,
            name='calculate_difference_spectrum'
        )

        # Switching Event Detected?
        switch_detected = tf_switch_detected(difference_spectrum, self.switch_threshold)
        
        if not switch_detected and not self.switching_flag:
            self.current_state_vector.assign(
                self.calculate_unknown_apparent_power(current_apparent_power=apparent_power)
                )
            return self.current_state_vector
                    
        if switch_detected:
            if not self.switching_flag:
                # Switching process start
                self.switching_flag.assign(True)
                self.base_spectrum.assign(current_background)
            
            # Skip step size
            self.n_frames_in_window.assign(
                tf.math.subtract(
                    x=self.n_frames_in_window,
                    y=self.step_size
                )            
            )
            self.current_state_vector.assign(
                self.calculate_unknown_apparent_power(current_apparent_power=apparent_power)
            )
            return self.current_state_vector
                
        # Switching process end
        
        # #++++++++++++++++++++++# #
        # #+++ Classification +++# #
        # #++++++++++++++++++++++# #
        
        # Calculate cleaned spectrum for classification
        clf_spectrum = tf.math.reduce_mean(
            input_tensor=self.window[:self.window_size],
            axis=0,
            name='calculate_mean_clf_spectrum'
        )
        cleaned_clf_spectrum = tf.math.subtract(
            x=clf_spectrum,
            y=self.base_spectrum,
            name='calculate_cleaned_clf_spectrum'
        )


        self.current_state_vector.assign(
            self.classify_switching_event(
                cleaned_spectrum=cleaned_clf_spectrum,
                current_apparent_power=apparent_power
            )
        )

        # Clear switching record to take account of the new baseline
        self.switching_flag.assign(False)
        self.base_spectrum.assign(tf.zeros(shape=self.fft_size_real,dtype=tf.float32))
        
        self.current_state_vector.assign(
            self.calculate_unknown_apparent_power(current_apparent_power=apparent_power)
        )
        return self.current_state_vector

    def predict(self, X: tf.Tensor) -> tf.Tensor:
        """
        Wrapper for call method

        Parameters
        ----------
        X

        Returns
        -------

        """
        return self.call(X)

    def crop_data_point(self, X: tf.Tensor) -> Tuple[tf.Tensor, tf.Tensor]:
        """
        One data point consists of multiple spectra and quantities. For this algorithm, however, only a subset is
        needed. This functions removes all unnecessary values from a data point.

        Parameters
        ----------
        X
            One data point.

        Returns
        -------
        cropped_data_point:
            Tuple: (spectrum, apparent_power)
        """
        i = self.spectrum_type * self.fft_size_real
        j = (self.spectrum_type + 1) * self.fft_size_real
        spectrum = X[i:j]
        apparent_power = X[-2]
        return spectrum, apparent_power
        
    def check_physical_condition_switch_on(self, appliance_index):
        """
        Check if switch-on event of known appliance satisfies the physical boundary conditions.

        Parameters
        ----------
        appliance_index
            Index of known appliance            

        Returns
        -------
        True of False
        """        
        if appliance_index < 0 or appliance_index >= self.n_known_appliances:
            return False
            
        if self.current_state_vector[appliance_index] != 0:
            tf.print("Device can only be switched on when it is off before")
            return False
            
        if self.check_power_diff_later:
            tf.print("!!!Warning: Last apparent power checking is not finished!!!") 
            return False
            
        # Calculate apparent power before switch
        self.power_before_switch.assign(
            tf.math.reduce_mean(
                input_tensor=self.power_window[(self.n_in_power_window - self.window_size):self.n_in_power_window]
            )
        )
        tf.print("*****Power before device", appliance_index, "switched on:", self.power_before_switch) 
        # Calculate apparent power after switch later
        self.check_power_diff_later.assign(True)
        self.skip_power_counter.assign(0) 
        self.skip_power_max.assign(self.get_check_power_diff_interval(appliance_index=appliance_index))
        self.switched_on_appliance_index.assign(appliance_index)       
        return True

    def check_physical_condition_switch_off(self, appliance_index):
        """
        Check if switch-off event of known appliance satisfies the physical boundary conditions.

        Parameters
        ----------
        appliance_index
            Index of known appliance

        Returns
        -------
        True of False
        """          
        if appliance_index < 0 or appliance_index >= self.n_known_appliances:
            return False
            
        if self.current_state_vector[appliance_index] == 0:
            tf.print("Device can only be switched off when it is on before")
            return False 

        if self.check_power_diff_later:
            tf.print("!!!Warning: Last apparent power checking is not finished!!!") 
            return False
                        
        # Calculate apparent power before switch
        self.power_before_switch.assign(
            tf.math.reduce_mean(
                input_tensor=self.power_window[(self.n_in_power_window - self.window_size):self.n_in_power_window]
            )
        )
        tf.print("*****Power before device", appliance_index, "switched off:", self.power_before_switch)      
        # Calculate apparent power after switch later
        self.check_power_diff_later.assign(True)
        self.skip_power_counter.assign(0) 
        self.skip_power_max.assign(self.get_check_power_diff_interval(appliance_index=appliance_index))
        self.switched_off_appliance_index.assign(appliance_index)       
        return True   
                        
    def calculate_state_vector(self,
                               event_class: tf.Tensor) -> tf.Tensor:
        """
        Given the current state vector and a classification results, return an updated state vector.

        Parameters
        ----------
        event_class
            One-hot encoded array of length 2N+1 (N is the number of known appliances).
        current_apparent_power
            Current, total value of apparent power            

        Returns
        -------
        updated_state_vector
        """

        # updated_state_vector = copy.deepcopy(self.current_state_vector)
        
        event_index = tf.math.argmax(event_class, output_type=tf.int32)
        tf.print("************************************event index is",event_index,"***********************")
        if self.check_phy_condition:
        # Calculate state vector considering physical boundary conditions  
            if self.is_noise(event_index):
                tf.print("Ignore noise")
                new_state_vector = self.current_state_vector
            else:             
                if event_index < self.n_known_appliances:
                # Known appliance is switched on 
                    appliance_index = event_index          
                    if self.check_physical_condition_switch_on(appliance_index=appliance_index):                    
                        new_state_vector = tf.tensor_scatter_nd_update(tensor=self.current_state_vector,
                            indices=[[appliance_index]],
                            updates=[self.apparent_power_list[appliance_index]])
                    else:
                        new_state_vector = self.current_state_vector
			
                elif self.n_known_appliances <= event_index < self.n_known_appliances * 2 :
                # Known appliance is switched off
                    appliance_index = tf.math.subtract(event_index, self.n_known_appliances)
                    if self.check_physical_condition_switch_off(appliance_index=appliance_index):
                        new_state_vector = tf.tensor_scatter_nd_update(tensor=self.current_state_vector,
                            indices=[[appliance_index]],
                            updates=[tf.constant(0, dtype=tf.float32)])
                    else:
                        new_state_vector = self.current_state_vector
                else:
                    new_state_vector = self.current_state_vector
        else:
         # Calculate state vector without considering physical boundary conditions         
            new_state_vector = tf.case(
                pred_fn_pairs=[
                # Case 1: Known appliance is switched on
                (tf.math.less(event_index, self.n_known_appliances),
                 lambda: tf.tensor_scatter_nd_update(tensor=self.current_state_vector,
                                                     indices=[[event_index]],
                                                     updates=[self.apparent_power_list[event_index]])),

                # Case 2: Known appliance is switched off
                (tf.math.logical_and(tf.math.greater_equal(event_index, self.n_known_appliances),
                                     tf.math.less(event_index, tf.math.multiply(self.n_known_appliances,
                                                                                tf.constant(2, dtype=tf.int32)))),
                 lambda: tf.tensor_scatter_nd_update(tensor=self.current_state_vector,
                                                     indices=[[tf.math.subtract(event_index, self.n_known_appliances)]],
                                                     updates=[tf.constant(0, dtype=tf.float32)])),
                ],
                default=lambda: self.current_state_vector
            )        	

        return new_state_vector
        
    def calculate_unknown_apparent_power(self, current_apparent_power: tf.Tensor) -> tf.Tensor:
        """
        Calculate an updated version of the state vector with an updated value of "unknown".

        Parameters
        ----------
        current_apparent_power
            Current, total value of apparent power

        Returns
        -------
        updated_state_vector
        """
        # updated_state_vector = copy.deepcopy(self.current_state_vector)
        known_power = tf.math.reduce_sum(self.current_state_vector[:-1])
        power_difference = tf.math.subtract(current_apparent_power, known_power)
        unknown_power = tf.math.maximum(power_difference,
                                        tf.constant(0, dtype=tf.float32))
        updated_state_vector = tf.concat([self.current_state_vector[:-1],
                                          tf.expand_dims(unknown_power, axis=0)],
                                         axis=0)
        return updated_state_vector

    def classify_switching_event(self,
                                 cleaned_spectrum: tf.Tensor,
                                 current_apparent_power: tf.Tensor) -> tf.Tensor:
        """
        Classify a switching event based on the cleaned spectrum.

        Parameters
        ----------
        cleaned_spectrum
            Array containing the cleaned spectrum.
        current_apparent_power
            Total of the apparent power.
        Returns
        -------
        updated_state_vector
        """
        # 1. Calculate feature vector
        self.feature_vector.assign(tf_calculate_feature_vector(cleaned_spectrum=cleaned_spectrum,
                                                               n_peaks_max=self.n_peaks_max,
                                                               fft_size_real=self.fft_size_real,
                                                               sample_rate=self.sample_rate))

        # 2. Classify event
        distances, event_class = self.call_knn(self.feature_vector)

        # If any of the distances is nan, do not change the state vector. Just return the current
        # state vector instead.
        if tf.math.equal(tf.math.reduce_any(tf.math.is_nan(distances)),
                         tf.constant(True)):
            return self.current_state_vector

        # Check if known or unknown event via the smallest distance
        event_class = tf.cond(
            pred=tf.math.greater(tf.math.reduce_min(distances), self.distance_threshold),
            true_fn=lambda: tf.one_hot(indices=2*self.n_known_appliances,
                                       depth=tf.math.add(2*self.n_known_appliances, tf.constant(1, dtype=tf.int32))),
            false_fn=lambda: event_class
        )

        # 3. Update current state vector accordingly
        self.current_state_vector.assign(self.calculate_state_vector(event_class=event_class))

        # 4. Update current state vector's power value for "unknown"
        self.current_state_vector.assign(
            self.calculate_unknown_apparent_power(current_apparent_power=current_apparent_power)
        )

        return self.current_state_vector
                                                         
    def is_noise(self, event_index: tf.Tensor):
    
        # After turn off power strip, the apparent power should be around 0, considering delay, set threshold to 4.
        if event_index == 14 and self.power_window[0] > 4:
            return True
        
        # Before turn on power strip, the apparent power should be around 0.   
        if event_index == 6 and self.power_window[self.n_in_power_window - 1] > 1:
            return True
        return False
                    
    def check_apparent_power_diff(self,
                                  current_apparent_power: tf.Tensor) -> None:
 
        # Counter
        self.skip_power_counter.assign(tf.add(self.skip_power_counter,1))    
           
        if tf.math.greater_equal(self.skip_power_counter,self.skip_power_max):
            self.power_after_switch.assign(tf.math.reduce_mean(
		                               input_tensor=self.power_window[:self.window_size]))                      
      
        if self.power_before_switch != -1 and self.power_after_switch != -1:
                                     
            if self.switched_on_appliance_index != -1:
                tf.print("*****Power after device", self.switched_on_appliance_index, "switched on:", self.power_after_switch)
                diff_power = tf.math.subtract(self.power_after_switch, self.power_before_switch)
                if self.is_power_diff_in_range(power_difference=diff_power, 
                                               appliance_index=self.switched_on_appliance_index):
                    tf.print("Power difference is in allowed range")
                else:
                    tf.print("Power difference out of range, device", 
                             self.switched_on_appliance_index,"is not on, roll back state vector" )             
                    # Roll back state vector
                    self.current_state_vector.assign(tf.tensor_scatter_nd_update(tensor=self.current_state_vector,
                                                                         indices=[[self.switched_on_appliance_index]],
                                                                         updates=[tf.constant(0, dtype=tf.float32)]))
                    # Update power value for unknown
                    self.current_state_vector.assign(
                    self.calculate_unknown_apparent_power(current_apparent_power=current_apparent_power)
                    ) 
                                                                                                                                           
                self.switched_on_appliance_index.assign(-1)
                    
            if self.switched_off_appliance_index != -1:
                tf.print("*****Power after device", self.switched_off_appliance_index, "switched off:", self.power_after_switch)
                diff_power = tf.math.subtract(self.power_before_switch, self.power_after_switch)  
                if self.is_power_diff_in_range(power_difference=diff_power, 
                                               appliance_index=self.switched_off_appliance_index):
                    tf.print("Power difference is in allowed range")
                else:
                    tf.print("Power difference out of range, device", 
                             self.switched_off_appliance_index,"is not off, roll back state vector" )             
                    # Roll back state vector
                    appliance_power = self.apparent_power_list[self.switched_off_appliance_index]
                    self.current_state_vector.assign(tf.tensor_scatter_nd_update(tensor=self.current_state_vector,
                                                                                 indices=[[self.switched_off_appliance_index]],
                                                                                 updates=[appliance_power]))
                    # Update power value for unknown
                    self.current_state_vector.assign(
                        self.calculate_unknown_apparent_power(current_apparent_power=current_apparent_power)
                    )                                      
                self.switched_off_appliance_index.assign(-1)
                
            self.check_power_diff_later.assign(False)
            self.power_before_switch.assign(-1)
            self.power_after_switch.assign(-1) 
        
        return None
    
    def get_check_power_diff_interval(self,
                                      appliance_index:tf.Tensor):
        # Default interval value for check the power difference
        default_interval = tf.constant(5 * self.window_size, dtype=tf.int32)  
        if appliance_index == 0:
            return tf.constant(10 * self.window_size, dtype=tf.int32)
        return  default_interval
                     
    def is_power_diff_in_range(self,
                               power_difference:tf.Tensor, 
                               appliance_index:tf.Tensor):
                                    
        # Check if the power difference is in allowed range
        appliance_power = self.apparent_power_list[appliance_index]
        power_threshold = appliance_power * self.apparent_power_threshold            
        if tf.math.logical_or(tf.math.greater(power_difference, tf.math.add(appliance_power, power_threshold)),
                              tf.math.less(power_difference, tf.math.subtract(appliance_power, power_threshold))):
            return False                                                                                     
        else:
            return True
                 
    def update_power_window(self, apparent_power) -> None:
        if self.check_phy_condition:
            self.power_window.assign(
                tf.tensor_scatter_nd_update(tensor=tf.roll(self.power_window, shift=1, axis=0),
                                            indices=[[0]],
                                            updates=[apparent_power])
            ) 
            if tf.math.less(self.n_in_power_window, self.window_size * 5):
                self.n_in_power_window.assign(
                    tf.add(self.n_in_power_window,
                           tf.constant(1, dtype=tf.int32)
                          )
                )   
        return None
        
    def update_window(self, input_tensor: tf.Tensor) -> None:
        """
        Function to add spectrum to window and, if necessary, increase frames counter.

        Parameters
        ----------
        input_tensor
            Spectrum
        """
        self.window_eviction(input_tensor)

        if tf.math.less(self.n_frames_in_window, self.window_size * 3):

            self.n_frames_in_window.assign(
                tf.add(
                    self.n_frames_in_window,
                    tf.constant(1, dtype=tf.int32)
                )
            )

        return None

    def window_eviction(self, input_tensor: tf.Tensor) -> None:
        """
        Function to replace the oldest spectrum in window with **input**.

        Parameters
        ----------
        input_tensor
            Spectrum to be added to the current window.
        """
        self.window.assign(
            tf.tensor_scatter_nd_update(
                tensor=tf.roll(input=self.window, shift=1, axis=0),
                indices=tf.constant([[0]], dtype=tf.int32),
                updates=tf.reshape(input_tensor, shape=(1, -1))
            )
        )
        return None

    def clear_window(self) -> None:
        """
        Clear window and reset current number of frames in window.
        """
        self.window.assign(
            tf.zeros(shape=(self.window_size * 3, self.fft_size_real))
        )

        self.n_frames_in_window.assign(
            tf.constant(value=0,
                        dtype=tf.int32)
        )
        return None

    def reset_internal_states(self) -> None:
        """
        Function to reset state vector, window and skipped frames variables.
        """
        self.clear_window()
        self.current_state_vector.assign(
            tf.zeros(shape=(self.n_known_appliances + 1), dtype=tf.float32)
        )
        return
